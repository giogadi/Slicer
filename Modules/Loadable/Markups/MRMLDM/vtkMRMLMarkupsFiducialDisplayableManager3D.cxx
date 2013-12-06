/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsFiducialDisplayableManager3D.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSeedWidget.h>
#include <vtkSmartPointer.h>
#include <vtkSeedRepresentation.h>
#include <vtkSphereSource.h>
#include <vtkSphereWidget2.h>
#include <vtkSphereRepresentation.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsFiducialDisplayableManager3D);
vtkCxxRevisionMacro (vtkMRMLMarkupsFiducialDisplayableManager3D, "$Revision: 1.0 $");

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager3D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsFiducialWidgetCallback3D : public vtkCommand
{
public:
  static vtkMarkupsFiducialWidgetCallback3D *New()
  { return new vtkMarkupsFiducialWidgetCallback3D; }

  vtkMarkupsFiducialWidgetCallback3D(){}

  virtual void Execute (vtkObject *vtkNotUsed(caller), unsigned long event, void *vtkNotUsed(callData))
  {
    if (event ==  vtkCommand::PlacePointEvent)
      {
      // std::cout << "Warning: PlacePointEvent not supported" << std::endl;
      }
    else if ((event == vtkCommand::EndInteractionEvent) || (event == vtkCommand::InteractionEvent))
      {
      // sanity checks
      if (!this->DisplayableManager)
        {
        return;
        }
      if (!this->Node)
        {
        return;
        }
      if (!this->Widget)
        {
        return;
        }
      // sanity checks end
      }

    if (event == vtkCommand::EndInteractionEvent)
      {
      // save the state of the node when done moving, then call
      // PropagateWidgetToMRML to update the node one last time
      if (this->Node->GetScene())
        {
        this->Node->GetScene()->SaveStateForUndo(this->Node);
        }
      }
    // the interaction with the widget ended, now propagate the changes to MRML
    this->DisplayableManager->PropagateWidgetToMRML(this->Widget, this->Node);
  }

  void SetWidget(vtkMRMLMarkupsDisplayableManager3D::NodeWidgets *w)
    {
    this->Widget = w;
    }
  void SetNode(vtkMRMLMarkupsNode *n)
    {
    this->Node = n;
    }
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager3D * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkMRMLMarkupsDisplayableManager3D::NodeWidgets * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager3D * DisplayableManager;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager3D methods
//---------------------------------------------------------------------------
vtkMRMLMarkupsFiducialDisplayableManager3D::vtkMRMLMarkupsFiducialDisplayableManager3D()
{
  this->Focus="vtkMRMLMarkupsFiducialNode";
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

namespace
{
  vtkAbstractWidget* createSeedWidget(vtkRenderWindowInteractor* interactor,
                                      vtkRenderer* renderer)
  {
    vtkNew<vtkSeedRepresentation> rep;
    vtkNew<vtkOrientedPolygonalHandleRepresentation3D> handle;

    // default to a starburst glyph, update in propagate mrml to widget
    vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
    glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::StarBurst2D);
    glyphSource->Update();
    glyphSource->SetScale(1.0);
    handle->SetHandle(glyphSource->GetOutput());


    rep->SetHandleRepresentation(handle.GetPointer());


    //seed widget
    vtkSeedWidget * seedWidget = vtkSeedWidget::New();

    seedWidget->SetRepresentation(rep.GetPointer());

    seedWidget->SetInteractor(interactor);
    seedWidget->SetCurrentRenderer(renderer);

    seedWidget->CompleteInteraction();

    return seedWidget;
  }

  vtkAbstractWidget* createSphereWidget(vtkRenderWindowInteractor* interactor,
                                        vtkRenderer* renderer)
  {
    vtkSphereWidget2* sphereWidget = vtkSphereWidget2::New();
    sphereWidget->CreateDefaultRepresentation();
    sphereWidget->SetInteractor(interactor);
    sphereWidget->SetCurrentRenderer(renderer);

    return sphereWidget;
  }
}

//---------------------------------------------------------------------------
/// Create a new widget.
vtkMRMLMarkupsDisplayableManager3D::NodeWidgets *
vtkMRMLMarkupsFiducialDisplayableManager3D::CreateWidget(vtkMRMLMarkupsNode* node)
{

  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return 0;
    }

  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);

  if (!fiducialNode)
    {
    vtkErrorMacro("CreateWidget: Could not get fiducial node!")
    return 0;
    }

  NodeWidgets* widget = new NodeWidgets;
  widget->NodeWidget = createSeedWidget(this->GetInteractor(), this->GetRenderer());

  // each markup's widget will be created on-the-fly somewhere in propagateMRMLToWidget()

  return widget;
}

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnWidgetCreated(NodeWidgets * widget, vtkMRMLMarkupsNode * node)
{

  if (!widget)
    {
    vtkErrorMacro("OnWidgetCreated: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("OnWidgetCreated: MRML node was null!")
    return;
    }

  // add the callback
  vtkMarkupsFiducialWidgetCallback3D *myCallback = vtkMarkupsFiducialWidgetCallback3D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->NodeWidget->AddObserver(vtkCommand::EndInteractionEvent,myCallback);
  widget->NodeWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  myCallback->Delete();

}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsFiducialDisplayableManager3D::UpdateNthSeedPositionFromMRML(int n, vtkSeedWidget *seedWidget, vtkMRMLMarkupsNode *pointsNode)
{
  if (!pointsNode || !seedWidget)
    {
    return false;
    }
  if (n > pointsNode->GetNumberOfMarkups())
    {
    return false;
    }
  vtkSeedRepresentation * seedRepresentation = vtkSeedRepresentation::SafeDownCast(seedWidget->GetRepresentation());
  if (!seedRepresentation)
    {
    return false;
    }
  bool positionChanged = false;

  // transform fiducial point using parent transforms
  double fidWorldCoord[4];
  pointsNode->GetMarkupPointWorld(n, 0, fidWorldCoord);

  // for 3d managers, compare world positions
  double seedWorldCoord[4];
  seedRepresentation->GetSeedWorldPosition(n,seedWorldCoord);

  if (this->GetWorldCoordinatesChanged(seedWorldCoord, fidWorldCoord))
    {
    vtkDebugMacro("UpdateNthSeedPositionFromMRML: 3D:"
                  << " world coordinates changed:\n\tseed = "
                  << seedWorldCoord[0] << ", "
                  << seedWorldCoord[1] << ", "
                  << seedWorldCoord[2] << "\n\tfid =  "
                  << fidWorldCoord[0] << ", "
                  << fidWorldCoord[1] << ", "
                  << fidWorldCoord[2]);
    seedRepresentation->GetHandleRepresentation(n)->SetWorldPosition(fidWorldCoord);
    positionChanged = true;
    }
  else
    {
    vtkDebugMacro("UpdateNthSeedPositionFromMRML: 3D: world coordinates unchanged!");
    }

  return positionChanged;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::SetNthSeed(int n, vtkMRMLMarkupsFiducialNode* fiducialNode, vtkSeedWidget *seedWidget)
{

  vtkSeedRepresentation * seedRepresentation = vtkSeedRepresentation::SafeDownCast(seedWidget->GetRepresentation());

  if (!seedRepresentation)
    {
    vtkErrorMacro("SetNthSeed: no seed representation in widget!");
    return;
    }

  vtkMRMLMarkupsDisplayNode *displayNode = fiducialNode->GetMarkupsDisplayNode();
  if (!displayNode)
    {
    vtkDebugMacro("SetNthSeed: Could not get display node for node " << (fiducialNode->GetID() ? fiducialNode->GetID() : "null id"));
    return;
    }

  int numberOfHandles = seedRepresentation->GetNumberOfSeeds();
  vtkDebugMacro("SetNthSeed, n = " << n << ", number of handles = " << numberOfHandles);

  // does this handle need to be created?
  bool createdNewHandle = false;
  if (n >= numberOfHandles)
    {
    // create a new handle
    vtkHandleWidget* newhandle = seedWidget->CreateNewHandle();
    if (!newhandle)
      {
      vtkErrorMacro("SetNthSeed: error creaing a new handle!");
      }
    else
      {
      // std::cout << "SetNthSeed: created a new handle,number of seeds = " << seedRepresentation->GetNumberOfSeeds() << std::endl;
      createdNewHandle = true;
      newhandle->ManagesCursorOff();
      if (newhandle->GetEnabled() == 0)
        {
        vtkDebugMacro("turning on the new handle");
        newhandle->EnabledOn();
        }
      }
    }

  // update the postion
  bool positionChanged = this->UpdateNthSeedPositionFromMRML(n, seedWidget, fiducialNode);
  if (!positionChanged)
    {
    vtkDebugMacro("Position did not change");
    }

  vtkOrientedPolygonalHandleRepresentation3D *handleRep =
    vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(seedRepresentation->GetHandleRepresentation(n));
  if (!handleRep)
    {
    vtkErrorMacro("Failed to get an oriented polygonal handle rep for n = "
          << n << ", number of seeds = "
          << seedRepresentation->GetNumberOfSeeds()
          << ", handle rep = "
          << (seedRepresentation->GetHandleRepresentation(n) ? seedRepresentation->GetHandleRepresentation(n)->GetClassName() : "null"));
    return;
    }

  // update the text
  std::string textString = fiducialNode->GetNthFiducialLabel(n);
  if (textString.compare(handleRep->GetLabelText()) != 0)
    {
    handleRep->SetLabelText(textString.c_str());
    }
  // visibility for this handle, hide it if the whole list is invisible,
  // this fid is invisible, or it's a 2d manager and the fids isn't
  // visible on this slice
  bool fidVisible = true;
  if (displayNode->GetVisibility() == 0 ||
      fiducialNode->GetNthFiducialVisibility(n) == 0 ||
      fiducialNode->GetFiducialMode() == vtkMRMLMarkupsFiducialNode::ORIENTATION_MODE)
    {
    fidVisible = false;
    }
  if (fidVisible)
    {
    handleRep->VisibilityOn();
    handleRep->HandleVisibilityOn();
    if (textString.compare("") != 0)
      {
      handleRep->LabelVisibilityOn();
      }
    seedWidget->GetSeed(n)->EnabledOn();
    }
  else
    {
    handleRep->VisibilityOff();
    handleRep->HandleVisibilityOff();
    handleRep->LabelVisibilityOff();
    seedWidget->GetSeed(n)->EnabledOff();
    }

  // TODO make this happen only once instead
  bool seedWidgetShouldBeVisible =
    displayNode->GetVisibility() != 0 &&
    fiducialNode->GetFiducialMode() == vtkMRMLMarkupsFiducialNode::POSITION_MODE;
  bool seedWidgetVisible = seedWidget->GetEnabled() != 0;
  if (seedWidgetVisible != seedWidgetShouldBeVisible)
    {
    if (seedWidgetShouldBeVisible)
      seedWidget->EnabledOn();
    else
      seedWidget->EnabledOff();

    seedWidget->CompleteInteraction();
    }

  // update locked
  int listLocked = fiducialNode->GetLocked();
  int seedLocked = fiducialNode->GetNthMarkupLocked(n);
  // if the user is placing lots of fiducials at once, add this one as locked
  // so that it can't be moved when placing the next fiducials. They will be
  // unlocked when the interaction node goes back into ViewTransform
  int persistentPlaceMode = 0;
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    persistentPlaceMode =
      (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      && (interactionNode->GetPlaceModePersistence() == 1);
    }
  if (listLocked || seedLocked || persistentPlaceMode)
    {
    seedWidget->GetSeed(n)->ProcessEventsOff();
    }
  else
    {
    seedWidget->GetSeed(n)->ProcessEventsOn();
    }


  // set the glyph type if a new handle was created, or the glyph type changed
  int oldGlyphType = this->Helper->GetNodeGlyphType(displayNode, n);
  if (createdNewHandle ||
      oldGlyphType != displayNode->GetGlyphType())
    {
    vtkDebugMacro("3D: DisplayNode glyph type = "
          << displayNode->GetGlyphType()
          << " = " << displayNode->GetGlyphTypeAsString()
          << ", is 3d glyph = "
          << (displayNode->GlyphTypeIs3D() ? "true" : "false"));
    if (displayNode->GlyphTypeIs3D())
      {
      if (displayNode->GetGlyphType() == vtkMRMLMarkupsDisplayNode::Sphere3D)
        {
        // std::cout << "3d sphere" << std::endl;
        vtkNew<vtkSphereSource> sphereSource;
        sphereSource->SetRadius(0.5);
        sphereSource->SetPhiResolution(10);
        sphereSource->SetThetaResolution(10);
        sphereSource->Update();
        handleRep->SetHandle(sphereSource->GetOutput());
        }
      else
        {
        // the 3d diamond isn't supported yet, use a 2d diamond for now
        vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
        glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Diamond2D);
        glyphSource->Update();
        glyphSource->SetScale(1.0);
        handleRep->SetHandle(glyphSource->GetOutput());
        }
      }//if (displayNode->GlyphTypeIs3D())
    else
      {
      // 2D
      vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
      glyphSource->SetGlyphType(displayNode->GetGlyphType());
      glyphSource->Update();
      glyphSource->SetScale(1.0);
      handleRep->SetHandle(glyphSource->GetOutput());
      }
    // TBD: keep with the assumption of one glyph type per markups node,
    // but they may have different glyphs during update
    this->Helper->SetNodeGlyphType(displayNode, displayNode->GetGlyphType(), n);
    }  // end of glyph type

  // update the text display properties if there is text
  if (textString.compare("") != 0)
    {
    // scale the text
    double textscale[3] = {displayNode->GetTextScale(), displayNode->GetTextScale(), displayNode->GetTextScale()};
    handleRep->SetLabelTextScale(textscale);
    if (handleRep->GetLabelTextActor())
      {
      // set the colours
      if (fiducialNode->GetNthFiducialSelected(n))
        {
        double *color = displayNode->GetSelectedColor();
        handleRep->GetLabelTextActor()->GetProperty()->SetColor(color);
        // std::cout << "Set label text actor property color to selected col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
        }
      else
        {
        double *color = displayNode->GetColor();
        handleRep->GetLabelTextActor()->GetProperty()->SetColor(color);
        // std::cout << "Set label text actor property color to col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
        }

      handleRep->GetLabelTextActor()->GetProperty()->SetOpacity(displayNode->GetOpacity());
      }
    }

  vtkProperty *prop = NULL;
  prop = handleRep->GetProperty();
  if (prop)
    {
    if (fiducialNode->GetNthFiducialSelected(n))
      {
      // use the selected color
      double *color = displayNode->GetSelectedColor();
      prop->SetColor(color);
      // std::cout << "Set glyph property color to selected col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
      }
    else
      {
      // use the unselected color
      double *color = displayNode->GetColor();
      prop->SetColor(color);
      // std::cout << "Set glyph property color to col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
      }

    // material properties
    prop->SetOpacity(displayNode->GetOpacity());
    prop->SetAmbient(displayNode->GetAmbient());
    prop->SetDiffuse(displayNode->GetDiffuse());
    prop->SetSpecular(displayNode->GetSpecular());
    }

  handleRep->SetUniformScale(displayNode->GetGlyphScale());

}

//---------------------------------------------------------------------------
/// Propagate properties of MRML node to widget.
void vtkMRMLMarkupsFiducialDisplayableManager3D::PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, NodeWidgets * widget)
{
  if (!widget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateMRMLToWidget: MRML node was null!")
    return;
    }

  // cast to the specific mrml node
  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);
  if (!fiducialNode)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get fiducial node!")
    return;
    }

  this->Updating = 1;

  // cast to the specific widget
  vtkSeedWidget* seedWidget = vtkSeedWidget::SafeDownCast(widget->NodeWidget);
  if (!seedWidget)
    {
    vtkErrorMacro("vtkMRMLMarkupsFiducialDisplayableManager3D::PropagateMRMLToWidget: no seed widget!");
    }
  else
    this->PropagateMRMLToSeedWidget(fiducialNode, seedWidget);

  this->PropagateMRMLToSphereWidgets(fiducialNode, widget);

  // update lock status
  this->UpdateLockedFromInteractionNode(fiducialNode);

  // TODO: see if this can't be fit into the individual propagate methods
  // DEBUG let's see if this fixes some issues
  // if (seedWidget)
  //   seedWidget->Modified();
  // for (WidgetListIt it = widget->MarkupWidgets.begin(); it != widget->MarkupWidgets.end(); ++it)
  //   {
  //   if (*it)
  //     {
  //     (*it)->Modified();
  //     vtkWidgetRepresentation* rep = (*it)->GetRepresentation();
  //     if (rep)
  //       rep->Modified();
  //     }
  //   }

  this->Updating = 0;
}

//---------------------------------------------------------------------------
/// Propagate properties of widget to MRML node.
void vtkMRMLMarkupsFiducialDisplayableManager3D::PropagateWidgetToMRML(NodeWidgets * widget, vtkMRMLMarkupsNode* node)
{

  if (!widget)
    {
    vtkErrorMacro("PropagateWidgetToMRML: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateWidgetToMRML: MRML node was null!")
    return;
    }

  // cast to the specific mrml node
  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);

  if (!fiducialNode)
   {
   vtkErrorMacro("PropagateWidgetToMRML: Could not get fiducial node!")
   return;
   }

  // Call corresponding propagate functions on each widget
  vtkSeedWidget* seedWidget = vtkSeedWidget::SafeDownCast(widget->NodeWidget);
  if (!seedWidget)
    {
    vtkErrorMacro("PropagateWidgetToMRML: no seed widget!");
    return;
    }
  this->PropagateSeedWidgetToMRML(seedWidget, fiducialNode);

  this->PropagateSphereWidgetsToMRML(widget->MarkupWidgets, fiducialNode);
}

//---------------------------------------------------------------------------
/// Create a markupsMRMLnode
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->Focus << ", jumping out");
    return;
    }

  // place the seed where the user clicked
  vtkDebugMacro("OnClickInRenderWindow: placing seed at " << x << ", " << y);
  // switch to updating state to avoid events mess
  this->Updating = 1;

  double displayCoordinates1[2];
  displayCoordinates1[0] = x;
  displayCoordinates1[1] = y;


  double worldCoordinates1[4];

  this->GetDisplayToWorldCoordinates(displayCoordinates1,worldCoordinates1);

  // Is there an active markups node that's a fiducial node?
  vtkMRMLMarkupsFiducialNode *activeFiducialNode = NULL;

  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsFiducialNode"))
      {
      activeFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsFiducialNode");
      }
    }

  bool newNode = false;
  if (!activeFiducialNode)
    {
    newNode = true;
    // create the MRML node
    activeFiducialNode = vtkMRMLMarkupsFiducialNode::New();
    activeFiducialNode->SetName("F");
    }

  // add a fiducial: this will trigger an update of the widgets
  int fiducialIndex = activeFiducialNode->AddMarkupWithNPoints(1);
  if (fiducialIndex == -1)
    {
    vtkErrorMacro("OnClickInRenderWindow: unable to add a fiducial to active fiducial list!");
    if (newNode)
      {
      activeFiducialNode->Delete();
      }
    return;
    }
  // set values on it
  activeFiducialNode->SetNthFiducialWorldCoordinates(fiducialIndex,worldCoordinates1);
  // std::cout << "OnClickInRenderWindow: Setting " << fiducialIndex << "th fiducial label from " << activeFiducialNode->GetNthFiducialLabel(fiducialIndex);

  // reset updating state
  this->Updating = 0;

  // if this was a one time place, go back to view transform mode
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode && interactionNode->GetPlaceModePersistence() != 1)
    {
    vtkDebugMacro("End of one time place, place mode persistence = " << interactionNode->GetPlaceModePersistence());
    interactionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
    }

  // save for undo and add the node to the scene after any reset of the
  // interaction node so that don't end up back in place mode
  this->GetMRMLScene()->SaveStateForUndo();

  // is there a node associated with this?
  if (associatedNodeID)
    {
    activeFiducialNode->SetNthFiducialAssociatedNodeID(fiducialIndex, associatedNodeID);
    }

  if (newNode)
    {
    // create a display node and add node and display node to scene
    vtkMRMLMarkupsDisplayNode *displayNode = vtkMRMLMarkupsDisplayNode::New();
    this->GetMRMLScene()->AddNode(displayNode);
    // let the logic know that it needs to set it to defaults
    displayNode->InvokeEvent(vtkMRMLMarkupsDisplayNode::ResetToDefaultsEvent);

    activeFiducialNode->AddAndObserveDisplayNodeID(displayNode->GetID());
    this->GetMRMLScene()->AddNode(activeFiducialNode);

    // have to reset the fid id since the fiducial node generates a scene
    // unique id only if the node was in the scene when the point was added
    if (!activeFiducialNode->ResetNthMarkupID(0))
      {
      vtkWarningMacro("Failed to reset the unique ID on the first fiducial in a new list: " << activeFiducialNode->GetNthMarkupID(0));
      }

    // save it as the active markups list
    if (selectionNode)
      {
      selectionNode->SetActivePlaceNodeID(activeFiducialNode->GetID());
      }
    // clean up
    displayNode->Delete();
    activeFiducialNode->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnInteractorStyleEvent(int eventid)
{
  this->Superclass::OnInteractorStyleEvent(eventid);

  if (this->GetDisableInteractorStyleEventsProcessing())
    {
    vtkWarningMacro("OnInteractorStyleEvent: Processing of events was disabled.")
    return;
    }

  if (eventid == vtkCommand::KeyPressEvent)
    {
    char *keySym = this->GetInteractor()->GetKeySym();
    vtkDebugMacro("OnInteractorStyleEvent 3D: key press event position = "
              << this->GetInteractor()->GetEventPosition()[0] << ", "
              << this->GetInteractor()->GetEventPosition()[1]
              << ", key sym = " << (keySym == NULL ? "null" : keySym));
    if (!keySym)
      {
      return;
      }
    if (strcmp(keySym, "p") == 0)
      {
      if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
        {
        this->OnClickInRenderWindowGetCoordinates();
        }
      else
        {
        vtkDebugMacro("Fiducial DisplayableManager: key press p, but not in Place mode! Returning.");
        return;
        }
      }
    }
  else if (eventid == vtkCommand::KeyReleaseEvent)
    {
    vtkDebugMacro("Got a key release event");
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnMRMLSceneEndClose()
{
  // TODO: IS THIS REALLY NECESSARY?
  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode* node, int n)
{
  int numberOfMarkups = node->GetNumberOfMarkups();
  if (n < 0 || n >= numberOfMarkups)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: n = " << n << " is out of range 0-" << numberOfMarkups);
    return;
    }

  NodeWidgets *widget = this->Helper->GetNodeWidgets(node);
  if (!widget)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: a markup was added to a node that doesn't already have a widget! Returning..");
    return;
    }

  vtkSeedWidget* seedWidget = vtkSeedWidget::SafeDownCast(widget->NodeWidget);
  if (!seedWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: Could not get seed widget!")
   return;
   }
  this->SetNthSeed(n, vtkMRMLMarkupsFiducialNode::SafeDownCast(node), seedWidget);

  // TODO make it only target the one widget
  this->PropagateMRMLToSphereWidgets(vtkMRMLMarkupsFiducialNode::SafeDownCast(node),
                                     this->Helper->GetNodeWidgets(node));
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode)
{
  vtkDebugMacro("OnMRMLMarkupsNodeMarkupAddedEvent");

  if (!markupsNode)
    {
    return;
    }
  NodeWidgets* nodeWidgets = this->Helper->GetNodeWidgets(markupsNode);
  if (!nodeWidgets)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: NodeWidget was null!");
    return;
    }

  vtkAbstractWidget* widget = nodeWidgets->NodeWidget;
  if (!widget)
    {
    // TBD: create a widget?
    vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: a markup was added to a node that doesn't already have a widget! Returning..");
    return;
    }

  vtkSeedWidget* seedWidget = vtkSeedWidget::SafeDownCast(widget);
  if (!seedWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: Could not get seed widget!")
   return;
   }

  // this call will create a new handle and set it
  int n = markupsNode->GetNumberOfMarkups() - 1;
  this->SetNthSeed(n, vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode), seedWidget);

  vtkSeedRepresentation * seedRepresentation = vtkSeedRepresentation::SafeDownCast(seedWidget->GetRepresentation());
  seedRepresentation->NeedToRenderOn();
  seedWidget->Modified();

  // Ensure a new sphere widget gets added as well
  this->PropagateMRMLToSphereWidgets(vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode),
                                     nodeWidgets);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode)
{
  vtkDebugMacro("OnMRMLMarkupsNodeMarkupRemovedEvent");

  if (!markupsNode)
    {
    return;
    }
  NodeWidgets* nodeWidgets = this->Helper->GetNodeWidgets(markupsNode);
  if (!nodeWidgets)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeMarkupRemovedEvent: nodeWidgets was null!");
    return;
    }

  vtkAbstractWidget *widget = nodeWidgets->NodeWidget;
  if (!widget)
    {
    // TBD: create a widget?
    vtkErrorMacro("OnMRMLMarkupsNodeMarkupRemovedEvent: a markup was removed from a node that doesn't already have a widget! Returning..");
    return;
    }

  // TODO: THIS IS HORRIBLE
  // for now, recreate the widget
  this->Helper->RemoveWidgetAndNode(markupsNode);
  this->AddWidget(markupsNode);
}

//---------------------------------------------------------------------------
namespace
{
  void GenerateArbitraryFrameFromZ(const double z[3], double x[3], double y[3])
  {
    double e1[] = {1.0, 0.0, 0.0};
    double e2[] = {0.0, 1.0, 0.0};

    double* startVec;
    if (fabs(vtkMath::Dot(z, e1) - 1.0) < 0.0000001)
      startVec = e2;
    else
      startVec = e1;

    double u[3];
    vtkMath::Cross(z, startVec, u);

    double v[3];
    vtkMath::Cross(z, u, v);

    // Populate x and y so that x,y,z are a right-handed frame
    vtkMath::Cross(u, v, startVec);
    if (vtkMath::Dot(z, startVec) < 0.0)
      {
      for (int i = 0; i < 3; ++i)
        {
        x[i] = v[i];
        y[i] = u[i];
        }
      }
    else
      {
      for (int i = 0; i < 3; ++i)
        {
        x[i] = u[i];
        y[i] = v[i];
        }
      }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::
PropagateSphereWidgetsToMRML(WidgetList& widgets, vtkMRMLMarkupsFiducialNode* node)
{
  if (widgets.size() != (unsigned) node->GetNumberOfFiducials())
    {
    vtkErrorMacro("PropagateSphereWidgetsToMRML: Nodes and widgets don't match up!");
    return;
    }

  for (unsigned wIdx = 0; wIdx < widgets.size(); ++wIdx)
    {
    vtkSphereWidget2* widget = vtkSphereWidget2::SafeDownCast(widgets[wIdx]);
    if (!widget)
      {
      vtkErrorMacro("PropagateSphereWidgetsToMRML: no sphere widget!");
      return;
      }

    vtkSphereRepresentation* sphereRep = vtkSphereRepresentation::SafeDownCast(widget->GetRepresentation());
    if (!sphereRep)
      {
      vtkDebugMacro("PropagateWidgetToMRML: widget's representation was not a sphere rep!");
      return;
      }

    this->Updating = 1;

    // We don't use GetHandleDirection because it's broken.
    double direction[3], pos[3], center[3];
    sphereRep->GetCenter(center);
    sphereRep->GetHandlePosition(pos);
    for (unsigned i = 0; i < 3; ++i)
      direction[i] = pos[i] - center[i];

    double x[3], y[3];
    GenerateArbitraryFrameFromZ(direction, x, y);

    double R[3][3];
    for (int i = 0; i < 3; ++i)
      {
      R[i][0] = x[i];
      R[i][1] = y[i];
      R[i][2] = direction[i];
      }
    double q[4];
    vtkMath::Matrix3x3ToQuaternion(R, q);

    int markupIdx = (int) wIdx;
    node->SetNthMarkupOrientationFromArray(markupIdx, q);

    node->Modified();

    node->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent,(void*)&markupIdx);

    this->Updating = 0;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::
PropagateSeedWidgetToMRML(vtkSeedWidget* seedWidget, vtkMRMLMarkupsFiducialNode* fiducialNode)
{
  if (seedWidget->GetWidgetState() != vtkSeedWidget::MovingSeed)
    {
    // ignore events not caused by seed movement
    return;
    }

  // disable processing of modified events
  this->Updating = 1;

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkSeedRepresentation * seedRepresentation = vtkSeedRepresentation::SafeDownCast(seedWidget->GetRepresentation());
  int numberOfSeeds = seedRepresentation->GetNumberOfSeeds();

  bool positionChanged = false;
  for (int n = 0; n < numberOfSeeds; n++)
    {
    double worldCoordinates1[4];
    seedRepresentation->GetSeedWorldPosition(n,worldCoordinates1);
    vtkDebugMacro("PropagateWidgetToMRML: 3d: widget seed " << n
          << " world coords = " << worldCoordinates1[0] << ", "
          << worldCoordinates1[1] << ", "<< worldCoordinates1[2]);

    // was there a change?
    double currentCoordinates[4];
    fiducialNode->GetNthFiducialWorldCoordinates(n,currentCoordinates);
    vtkDebugMacro("PropagateWidgetToMRML: fiducial " << n
          << " current world coordinates = " << currentCoordinates[0]
          << ", " << currentCoordinates[1] << ", "
          << currentCoordinates[2]);

    double currentCoords[3];
    currentCoords[0] = currentCoordinates[0];
    currentCoords[1] = currentCoordinates[1];
    currentCoords[2] = currentCoordinates[2];
    double newCoords[3];
    newCoords[0] = worldCoordinates1[0];
    newCoords[1] = worldCoordinates1[1];
    newCoords[2] = worldCoordinates1[2];
    if (this->GetWorldCoordinatesChanged(currentCoords, newCoords))
      {
      vtkDebugMacro("PropagateWidgetToMRML: position changed.");
      positionChanged = true;
      }

    if (positionChanged)
      {
      vtkDebugMacro("PropagateWidgetToMRML: position changed, setting fiducial coordinates");
      fiducialNode->SetNthFiducialWorldCoordinates(n,worldCoordinates1);
      }
    }

  // did any of the positions change?
  if (positionChanged)
    {
    vtkDebugMacro("PropagateWidgetToMRML: position changed, calling point modified on the fiducial node");
    fiducialNode->Modified();
    fiducialNode->GetScene()->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent,fiducialNode);
    }
  // This displayableManager should now consider ModifiedEvent again
  this->Updating = 0;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::
PropagateMRMLToSphereWidgets(vtkMRMLMarkupsFiducialNode* node, NodeWidgets* nodeWidgets)
{
  WidgetList& widgets = nodeWidgets->MarkupWidgets;
  // If fiducials have been added to this node, create corresponding
  // widgets
  if ((unsigned) node->GetNumberOfFiducials() > widgets.size())
    {
    vtkDebugMacro("PropagateMRMLToSphereWidgets: Node fiducial count doesn't match widget count!");
    vtkDebugMacro("Fiducials: " << node->GetNumberOfFiducials() << " Widgets: " << widgets.size());

    for (int i = (int) widgets.size(); i < node->GetNumberOfFiducials(); ++i)
      {
      vtkAbstractWidget* sphereWidget =
        createSphereWidget(this->GetInteractor(), this->GetRenderer());
      vtkMarkupsFiducialWidgetCallback3D *myCallback = vtkMarkupsFiducialWidgetCallback3D::New();
      myCallback->SetNode(node);
      myCallback->SetWidget(nodeWidgets);
      myCallback->SetDisplayableManager(this);
      sphereWidget->AddObserver(vtkCommand::EndInteractionEvent,myCallback);
      sphereWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);
      myCallback->Delete();
      widgets.push_back(sphereWidget);
      }
    }
  else if ((unsigned) node->GetNumberOfFiducials() < widgets.size())
    {
    vtkErrorMacro("ERROR: WHY ARE THERE MORE WIDGETS THAN FIDUCIALS?!?!?!");
    return;
    }

  for (unsigned wIdx = 0; wIdx < widgets.size(); ++wIdx)
    {
    vtkSphereWidget2* w = vtkSphereWidget2::SafeDownCast(widgets[wIdx]);
    if (!w)
      {
      vtkErrorMacro("PropagateMRMLToSphereWidgets: widget " << wIdx << " is NULL!");
      return;
      }

    vtkSphereRepresentation* sphereRep = vtkSphereRepresentation::SafeDownCast(w->GetRepresentation());
    if (!sphereRep)
      {
      vtkErrorMacro("PropagateMRMLToSphereWidgets: widget " << wIdx << "'s representation was not a sphere representation!");
      return;
      }

    // Set center of widget
    double p[3];
    node->GetNthFiducialPosition((int) wIdx, p);
    sphereRep->SetCenter(p);

    // Set direction of widget
    double q[4];
    node->GetNthMarkupOrientation((int) wIdx, q);
    double R[3][3];
    vtkMath::QuaternionToMatrix3x3(q, R);

    // We do this really stupidly because vtkSphereWidget2's
    // SetHandleDirection is broken
    sphereRep->SetHandlePosition(p[0]+R[0][2], p[1]+R[1][2], p[2]+R[2][2]);

    // DEBUG
    // double u[3];
    // sphereRep->GetCenter(u);
    // std::cout << wIdx << " " << u[0] << " " << u[1] << " " << u[2] << " ";
    // sphereRep->GetHandlePosition(u);
    // std::cout << u[0] << " " << u[1] << " " << u[2] << " ";
    // std::cout << R[0][2] << " " << R[1][2] << " " << R[2][2] << std::endl;

    // TODO check if we really need to make the widget
    // visible/invisible or just toggle enabled
    bool isNodeVisible = node->GetDisplayNode()->GetVisibility() != 0;
    bool isMarkupVisible = node->GetNthFiducialVisibility(wIdx);
    bool shouldBeVisible =
      isNodeVisible &&
      isMarkupVisible &&
      node->GetFiducialMode() == vtkMRMLMarkupsFiducialNode::ORIENTATION_MODE;
    bool isWidgetVisible = w->GetEnabled() != 0;
    if (isWidgetVisible != shouldBeVisible)
      {
      if (shouldBeVisible)
        {
        sphereRep->VisibilityOn();
        sphereRep->HandleVisibilityOn();
        w->EnabledOn();
        }
      else
        {
        sphereRep->VisibilityOff();
        sphereRep->HandleVisibilityOff();
        w->EnabledOff();
        }
      }

    // TODO do we need to also update locked status?
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::
PropagateMRMLToSeedWidget(vtkMRMLMarkupsFiducialNode* fiducialNode, vtkSeedWidget* seedWidget)
{
  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkMRMLMarkupsDisplayNode *displayNode = fiducialNode->GetMarkupsDisplayNode();

  if (!displayNode)
    {
    vtkDebugMacro("PropagateMRMLToWidget: Could not get display node for node " << (fiducialNode->GetID() ? fiducialNode->GetID() : "null id"));
    }

  // iterate over the fiducials in this markup
  int numberOfFiducials = fiducialNode->GetNumberOfMarkups();

  vtkDebugMacro("Fids PropagateMRMLToWidget, node num markups = " << numberOfFiducials);

  for (int n = 0; n < numberOfFiducials; n++)
    {
    this->SetNthSeed(n, fiducialNode, seedWidget);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::UpdateNodeWidgetsLocks(vtkMRMLMarkupsNode* node, bool isLocked)
{
  if (!node)
    {
    vtkErrorMacro("vtkMRMLMarkupsFiducialDisplayableManager3D::UpdateNodeWidgetsLocks: node is null!");
    return;
    }

  NodeWidgets* widget = this->Helper->GetNodeWidgets(node);
  if (!widget)
    {
    vtkErrorMacro("vtkMRMLMarkupsFiducialDisplayableManager3D::UpdateNodeWidgetsLocks: node widget is null!");
    return;
    }

  if (!widget->NodeWidget)
    {
    vtkErrorMacro("vtkMRMLMarkupsFiducialDisplayableManager3D::UpdateNodeWidgetsLocks: seed widget is null!");
    return;
    }

  if (widget->MarkupWidgets.size() != (unsigned) node->GetNumberOfMarkups())
    {
    vtkErrorMacro("vtkMRMLMarkupsFiducialDisplayableManager3D::UpdateNodeWidgetsLocks: node fiducials and widgets don't match up!");
    return;
    }

  // First update locked status of seed widget
  vtkSeedWidget* seedWidget = vtkSeedWidget::SafeDownCast(widget->NodeWidget);
  if (!seedWidget)
    {
    vtkErrorMacro("UpdateNodeWidgetsLocks: no seed widget!");
    return;
    }

  bool seedWidgetLocked = seedWidget->GetProcessEvents() == 0;
  if (isLocked != seedWidgetLocked)
    {
    if (isLocked)
      seedWidget->ProcessEventsOff();
    else
      {
      // Since individual markups might be locked, lock individual seeds
      seedWidget->ProcessEventsOn();

      int numMarkups = node->GetNumberOfMarkups();
      for (int i = 0; i < numMarkups; ++i)
        {
        if (seedWidget->GetSeed(i) == NULL)
          {
          vtkErrorMacro("UpdateLocked: missing seed at index " << i);
          continue;
          }
        bool isLockedOnNthMarkup = node->GetNthMarkupLocked(i);
        bool isLockedOnNthSeed = seedWidget->GetSeed(i)->GetProcessEvents() == 0;
        if (isLockedOnNthMarkup && !isLockedOnNthSeed)
          {
          // lock it
          seedWidget->GetSeed(i)->ProcessEventsOff();
          }
        else if (!isLockedOnNthMarkup && isLockedOnNthSeed)
          {
          // unlock it
          seedWidget->GetSeed(i)->ProcessEventsOn();
          }
        }
      }
    }

  // Update locked status for sphere widgets
  int numMarkups = node->GetNumberOfMarkups();
  for (int i = 0; i < numMarkups; ++i)
    {
    if (seedWidget->GetSeed(i) == NULL)
      {
      vtkErrorMacro("UpdateLocked: missing seed at index " << i);
      //continue;
      }
    bool sphereWidgetLocked = widget->MarkupWidgets[i]->GetProcessEvents() == 0;
    bool shouldBeLocked = isLocked || node->GetNthMarkupLocked(i);
    if (sphereWidgetLocked != shouldBeLocked)
      {
      if (shouldBeLocked)
        widget->MarkupWidgets[i]->ProcessEventsOff();
      else
        widget->MarkupWidgets[i]->ProcessEventsOn();
      }
    }
}
