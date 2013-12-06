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

#ifndef __vtkMRMLMarkupsFiducialDisplayableManager3D_h
#define __vtkMRMLMarkupsFiducialDisplayableManager3D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManager3D.h"

class vtkMRMLMarkupsFiducialNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkTextWidget;
class vtkSphereWidget2;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsFiducialDisplayableManager3D :
    public vtkMRMLMarkupsDisplayableManager3D
{
public:

  static vtkMRMLMarkupsFiducialDisplayableManager3D *New();
  vtkTypeRevisionMacro(vtkMRMLMarkupsFiducialDisplayableManager3D, vtkMRMLMarkupsDisplayableManager3D);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkMRMLMarkupsFiducialDisplayableManager3D();
  virtual ~vtkMRMLMarkupsFiducialDisplayableManager3D(){}

  // OVERRIDE
  virtual void UpdateNodeWidgetsLocks(vtkMRMLMarkupsNode* node, bool isLocked);

  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y, const char *associatedNodeID);
  /// Create a widget.
  virtual NodeWidgets * CreateWidget(vtkMRMLMarkupsNode* node);
  /// Create new handle on widget when a new markup is added to a markups node
  virtual void OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode);
  /// Respond to the nth markup modified event
  virtual void OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode * markupsNode, int n);
  /// Respond to a markup being removed from the markups node
  virtual void OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode);

  /// Gets called when widget was created
  virtual void OnWidgetCreated(NodeWidgets * widget, vtkMRMLMarkupsNode * node);

  /// Update a single seed from MRML
  void SetNthSeed(int n, vtkMRMLMarkupsFiducialNode* fiducialNode, vtkSeedWidget *seedWidget);
  /// Propagate properties of MRML node to widget.
  virtual void PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, NodeWidgets * widget);

  /// Propagate properties of widget to MRML node.
  virtual void PropagateWidgetToMRML(NodeWidgets * widget, vtkMRMLMarkupsNode* node);

  /// Respond to the interactor style event
  virtual void OnInteractorStyleEvent(int eventid);

  /// Update a single seed position from the node, return true if the position changed
  virtual bool UpdateNthSeedPositionFromMRML(int n, vtkSeedWidget *widget, vtkMRMLMarkupsNode *pointsNode);

  // Clean up when scene closes
  virtual void OnMRMLSceneEndClose();

  void PropagateSphereWidgetsToMRML(WidgetList& widgets, vtkMRMLMarkupsFiducialNode* node);
  void PropagateSeedWidgetToMRML(vtkSeedWidget* widget, vtkMRMLMarkupsFiducialNode* node);

  void PropagateMRMLToSeedWidget(vtkMRMLMarkupsFiducialNode* node, vtkSeedWidget* widget);
  void PropagateMRMLToSphereWidgets(vtkMRMLMarkupsFiducialNode* node, NodeWidgets* widgets);

  void AfterPropagateMRMLToWidget(vtkMRMLMarkupsFiducialNode* node);

  // TRYING THIS OUT sorry mother
  bool PropagatingWidgetToMRML;

private:

  vtkMRMLMarkupsFiducialDisplayableManager3D(const vtkMRMLMarkupsFiducialDisplayableManager3D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsFiducialDisplayableManager3D&); /// Not Implemented
};

#endif
