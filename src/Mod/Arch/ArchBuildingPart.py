# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD
import Draft
import ArchCommands
import DraftVecUtils
import sys
import ArchIFC
import tempfile
import os
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond
if sys.version_info.major >= 3:
    unicode = str

## @package ArchBuildingPart
#  \ingroup ARCH
#  \brief The BuildingPart object and tools
#
#  This module provides tools to build BuildingPart objects.
#  BuildingParts are used to group different Arch objects

__title__="FreeCAD Arch BuildingPart"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


BuildingTypes = ['Undefined',
'Agricultural - Barn',
'Agricultural - Chicken coop or chickenhouse',
'Agricultural - Cow-shed',
'Agricultural - Farmhouse',
'Agricultural - Granary',
'Agricultural - Greenhouse',
'Agricultural - Hayloft',
'Agricultural - Pigpen or sty',
'Agricultural - Root cellar',
'Agricultural - Shed',
'Agricultural - Silo',
'Agricultural - Stable',
'Agricultural - Storm cellar',
'Agricultural - Well house',
'Agricultural - Underground pit',

'Commercial - Automobile repair shop',
'Commercial - Bank',
'Commercial - Car wash',
'Commercial - Convention center',
'Commercial - Forum',
'Commercial - Gas station',
'Commercial - Hotel',
'Commercial - Market',
'Commercial - Market house',
'Commercial - Skyscraper',
'Commercial - Shop',
'Commercial - Shopping mall',
'Commercial - Supermarket',
'Commercial - Warehouse',
'Commercial - Restaurant',

'Residential - Apartment block',
'Residential - Asylum',
'Residential - Condominium',
'Residential - Dormitory',
'Residential - Duplex',
'Residential - House',
'Residential - Nursing home',
'Residential - Townhouse',
'Residential - Villa',
'Residential - Bungalow',

'Educational - Archive',
'Educational - College classroom building',
'Educational - College gymnasium',
'Educational - College students union',
'Educational - School',
'Educational - Library',
'Educational - Museum',
'Educational - Art gallery',
'Educational - Theater',
'Educational - Amphitheater',
'Educational - Concert hall',
'Educational - Cinema',
'Educational - Opera house',
'Educational - Boarding school',

'Government - Capitol',
'Government - City hall',
'Government - Consulate',
'Government - Courthouse',
'Government - Embassy',
'Government - Fire station',
'Government - Meeting house',
'Government - Moot hall',
'Government - Palace',
'Government - Parliament',
'Government - Police station',
'Government - Post office',
'Government - Prison',

'Industrial - Brewery',
'Industrial - Factory',
'Industrial - Foundry',
'Industrial - Power plant',
'Industrial - Mill',

'Military - Arsenal',
'Military -Barracks',

'Parking - Boathouse',
'Parking - Garage',
'Parking - Hangar',

'Storage - Silo',
'Storage - Hangar',

'Religious - Church',
'Religious - Basilica',
'Religious - Cathedral',
'Religious - Chapel',
'Religious - Oratory',
'Religious - Martyrium',
'Religious - Mosque',
'Religious - Mihrab',
'Religious - Surau',
'Religious - Imambargah',
'Religious - Monastery',
'Religious - Mithraeum',
'Religious - Fire temple',
'Religious - Shrine',
'Religious - Synagogue',
'Religious - Temple',
'Religious - Pagoda',
'Religious - Gurdwara',
'Religious - Hindu temple',

'Transport - Airport terminal',
'Transport - Bus station',
'Transport - Metro station',
'Transport - Taxi station',
'Transport - Railway station',
'Transport - Signal box',
'Transport - Lighthouse',

'Infrastructure - Data centre',

'Power station - Fossil-fuel power station',
'Power station - Nuclear power plant',
'Power station - Geothermal power',
'Power station - Biomass-fuelled power plant',
'Power station - Waste heat power plant',
'Power station - Renewable energy power station',
'Power station - Atomic energy plant',

'Other - Apartment',
'Other - Clinic',
'Other - Community hall',
'Other - Eatery',
'Other - Folly',
'Other - Food court',
'Other - Hospice',
'Other - Hospital',
'Other - Hut',
'Other - Bathhouse',
'Other - Workshop',
'Other - World trade centre'
]


def makeBuildingPart(objectslist=None,baseobj=None,name="BuildingPart"):

    '''makeBuildingPart(objectslist): creates a buildingPart including the
    objects from the given list.'''

    obj = FreeCAD.ActiveDocument.addObject("App::GeometryPython","BuildingPart")
    #obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","BuildingPart")
    obj.Label = translate("Arch","BuildingPart")
    BuildingPart(obj)
    # if no IfcType is set it will be the first in the available
    # Annotation in IFC2x3 and Actuator in IFC4, both is certainly wrong
    # use Undefined ATM
    obj.IfcType = "Undefined"
    if FreeCAD.GuiUp:
        ViewProviderBuildingPart(obj.ViewObject)
    if objectslist:
        obj.addObjects(objectslist)
    return obj


def makeFloor(objectslist=None,baseobj=None,name="Floor"):

    """overwrites ArchFloor.makeFloor"""

    obj = makeBuildingPart(objectslist)
    obj.Label = name
    obj.IfcType = "Building Storey"
    return obj


def makeBuilding(objectslist=None,baseobj=None,name="Building"):

    """overwrites ArchBuilding.makeBuilding"""

    obj = makeBuildingPart(objectslist)
    obj.Label = name
    obj.IfcType = "Building"
    obj.addProperty("App::PropertyEnumeration","BuildingType","Building",QT_TRANSLATE_NOOP("App::Property","The type of this building"))
    obj.BuildingType = BuildingTypes
    if FreeCAD.GuiUp:
        obj.ViewObject.ShowLevel = False
        obj.ViewObject.ShowLabel = False
    return obj


def convertFloors(floor=None):

    """convert the given Floor or Building (or all Arch Floors from the active document if none is given) into BuildingParts"""

    todel = []
    if floor:
        objset = [floor]
    else:
        objset = FreeCAD.ActiveDocument.Objects
    for obj in objset:
        if Draft.getType(obj) in ["Floor","Building"]:
            nobj = makeBuildingPart(obj.Group)
            if Draft.getType(obj) == "Floor":
                nobj.IfcType = "Building Storey"
            else:
                nobj.IfcType = "Building"
                nobj.addProperty("App::PropertyEnumeration","BuildingType","Building",QT_TRANSLATE_NOOP("App::Property","The type of this building"))
                nobj.BuildingType = BuildingTypes
            label = obj.Label
            for parent in obj.InList:
                if hasattr(parent,"Group"):
                    if obj in parent.Group:
                        parent.addObject(nobj)
                        #g = parent.Group
                        #g.append(nobj)
                        #parent.Group = g
                else:
                    print("Warning: couldn't add new object '"+label+"' to parent object '"+parent.Label+"'")
            todel.append(obj.Name)
            if obj.ViewObject:
                obj.ViewObject.Proxy.Object = None # some bug makes this trigger even efter the object has been deleted...
            obj.Label = obj.Label+" to delete" # in case FreeCAD doesn't allow 2 objs with same label
            nobj.Label = label
    for n in todel:
        from DraftGui import todo
        todo.delay(FreeCAD.ActiveDocument.removeObject,n)



class CommandBuildingPart:


    "the Arch BuildingPart command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_BuildingPart',
                'MenuText': QT_TRANSLATE_NOOP("Arch_BuildingPart","BuildingPart"),
                'Accel': "B, P",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_BuildingPart","Creates a BuildingPart object including selected objects")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        ss = "[ "
        for o in sel:
            ss += "FreeCAD.ActiveDocument." + o.Name + ", "
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create BuildingPart"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("obj = Arch.makeBuildingPart("+ss+")")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()



class BuildingPart(ArchIFC.IfcProduct):


    "The BuildingPart object"

    def __init__(self,obj):

        obj.Proxy = self
        obj.addExtension('App::GroupExtensionPython', self)
        #obj.addExtension('App::OriginGroupExtensionPython', self)
        self.setProperties(obj)

    def setProperties(self,obj):
        ArchIFC.IfcProduct.setProperties(self, obj)

        pl = obj.PropertiesList
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The height of this object"))
        if not "HeightPropagate" in pl:
            obj.addProperty("App::PropertyBool","HeightPropagate","Children",QT_TRANSLATE_NOOP("App::Property","If true, the height value propagates to contained objects"))
            obj.HeightPropagate = True
        if not "LevelOffset" in pl:
            obj.addProperty("App::PropertyLength","LevelOffset","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The level of the (0,0,0) point of this level"))
        if not "Area" in pl:
            obj.addProperty("App::PropertyArea","Area", "BuildingPart",QT_TRANSLATE_NOOP("App::Property","The computed floor area of this floor"))
        if not "Description" in pl:
            obj.addProperty("App::PropertyString","Description","Component",QT_TRANSLATE_NOOP("App::Property","An optional description for this component"))
        if not "Tag" in pl:
            obj.addProperty("App::PropertyString","Tag","Component",QT_TRANSLATE_NOOP("App::Property","An optional tag for this component"))
        if not "Shape" in pl:
            obj.addProperty("Part::PropertyPartShape","Shape","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The shape of this object"))
        if not "SavedInventor" in pl:
            obj.addProperty("App::PropertyFileIncluded","SavedInventor","BuildingPart",QT_TRANSLATE_NOOP("App::Property","This property stores an inventor representation for this object"))
            obj.setEditorMode("SavedInventor",2)

        self.Type = "BuildingPart"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def onBeforeChange(self,obj,prop):

        if prop == "Placement":
            self.oldPlacement = FreeCAD.Placement(obj.Placement)

    def onChanged(self,obj,prop):

        ArchIFC.IfcProduct.onChanged(self, obj, prop)

        # clean svg cache if needed
        if prop in ["Placement","Group"]:
            self.svgcache = None
            self.shapecache = None

        if (prop == "Height" or prop == "HeightPropagate") and obj.Height.Value:
            self.touchChildren(obj)

        elif prop == "Placement":
            if hasattr(self,"oldPlacement"):
                if self.oldPlacement and (self.oldPlacement != obj.Placement):
                    deltap = obj.Placement.Base.sub(self.oldPlacement.Base)
                    if deltap.Length == 0:
                        deltap = None
                    v = FreeCAD.Vector(0,0,1)
                    deltar = FreeCAD.Rotation(self.oldPlacement.Rotation.multVec(v),obj.Placement.Rotation.multVec(v))
                    #print "Rotation",deltar.Axis,deltar.Angle
                    if deltar.Angle < 0.0001:
                        deltar = None
                    for child in obj.Group:
                        if ((not hasattr(child,"MoveWithHost")) or child.MoveWithHost) and hasattr(child,"Placement"):
                            #print "moving ",child.Label
                            if deltar:
                                #child.Placement.Rotation = child.Placement.Rotation.multiply(deltar) - not enough, child must also move
                                # use shape methods to obtain a correct placement
                                import Part,math
                                shape = Part.Shape()
                                shape.Placement = child.Placement
                                #print("angle before rotation:",shape.Placement.Rotation.Angle)
                                #print("rotation angle:",math.degrees(deltar.Angle))
                                shape.rotate(DraftVecUtils.tup(obj.Placement.Base), DraftVecUtils.tup(deltar.Axis), math.degrees(deltar.Angle))
                                print("angle after rotation:",shape.Placement.Rotation.Angle)
                                child.Placement = shape.Placement
                            if deltap:
                                print("moving child")
                                child.Placement.move(deltap)

    def execute(self,obj):

        # gather all the child shapes into a compound
        shapes = self.getShapes(obj)
        if shapes:
            f = []
            for s in shapes:
                f.extend(s.Faces)
            #print("faces before compound:",len(f))
            import Part
            obj.Shape = Part.makeCompound(f)
            #print("faces after compound:",len(obj.Shape.Faces))
            #print("recomputing ",obj.Label)
        obj.Area = self.getArea(obj)

    def getArea(self,obj):

        "computes the area of this floor by adding its inner spaces"

        area = 0
        if hasattr(obj,"Group"):
            for child in obj.Group:
                if hasattr(child,"Area") and hasattr(child,"IfcType"):
                    # only add arch objects that have an Area property
                    # TODO only spaces? ATM only spaces and windows have an Area property
                    area += child.Area.Value
        return area

    def getShapes(self,obj):

        "recursively get the shapes of objects inside this BuildingPart"

        shapes = []
        for child in Draft.getGroupContents(obj):
            if hasattr(child,'Shape'):
                shapes.extend(child.Shape.Faces)
        return shapes

    def getSpaces(self,obj):

        "gets the list of Spaces that have this object as their Zone property"

        g = []
        for o in obj.OutList:
            if hasattr(o,"Zone"):
                if o.Zone == obj:
                    g.append(o)
        return g

    def touchChildren(self,obj):
        
        "Touches all descendents where applicable"

        for child in obj.Group:
            if Draft.getType(child) in ["Wall","Structure"]:
                if not child.Height.Value:
                    print("Executing ",child.Label)
                    child.Proxy.execute(child)
            elif Draft.getType(child) in ["Group","BuildingPart"]:
                self.touchChildren(child)


class ViewProviderBuildingPart:


    "A View Provider for the BuildingPart object"

    def __init__(self,vobj):

        vobj.addExtension("Gui::ViewProviderGroupExtensionPython", self)
        #vobj.addExtension("Gui::ViewProviderGeoFeatureGroupExtensionPython", self)
        vobj.Proxy = self
        self.setProperties(vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Helpers")

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "LineWidth" in pl:
            vobj.addProperty("App::PropertyFloat","LineWidth","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The line width of this object"))
            vobj.LineWidth = 1
        if not "OverrideUnit" in pl:
            vobj.addProperty("App::PropertyString","OverrideUnit","BuildingPart",QT_TRANSLATE_NOOP("App::Property","An optional unit to express levels"))
        if not "DisplayOffset" in pl:
            vobj.addProperty("App::PropertyPlacement","DisplayOffset","BuildingPart",QT_TRANSLATE_NOOP("App::Property","A transformation to apply to the level mark"))
            vobj.DisplayOffset = FreeCAD.Placement(FreeCAD.Vector(0,0,0),FreeCAD.Rotation(FreeCAD.Vector(1,0,0),90))
        if not "ShowLevel" in pl:
            vobj.addProperty("App::PropertyBool","ShowLevel","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, show the level"))
            vobj.ShowLevel = True
        if not "ShowUnit" in pl:
            vobj.addProperty("App::PropertyBool","ShowUnit","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, show the unit on the level tag"))
        if not "OriginOffset" in pl:
            vobj.addProperty("App::PropertyBool","OriginOffset","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, display offset will affect the origin mark too"))
        if not "ShowLabel" in pl:
            vobj.addProperty("App::PropertyBool","ShowLabel","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, the object's label is displayed"))
            vobj.ShowLabel = True
        if not "FontName" in pl:
            vobj.addProperty("App::PropertyFont","FontName","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The font to be used for texts"))
            vobj.FontName = Draft.getParam("textfont","Arial")
        if not "FontSize" in pl:
            vobj.addProperty("App::PropertyLength","FontSize","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The font size of texts"))
            vobj.FontSize = Draft.getParam("textheight",2.0)
        if not "DiffuseColor" in pl:
            vobj.addProperty("App::PropertyColorList","DiffuseColor","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The individual face colors"))

        # Interaction properties
        if not "SetWorkingPlane" in pl:
            vobj.addProperty("App::PropertyBool","SetWorkingPlane","Interaction",QT_TRANSLATE_NOOP("App::Property","If true, when activated, the working plane will automatically adapt to this level"))
            vobj.SetWorkingPlane = True
        if not "AutoWorkingPlane" in pl:
            vobj.addProperty("App::PropertyBool","AutoWorkingPlane","Interaction",QT_TRANSLATE_NOOP("App::Property","If set to True, the working plane will be kept on Auto mode"))
        if not "ViewData" in pl:
            vobj.addProperty("App::PropertyFloatList","ViewData","Interaction",QT_TRANSLATE_NOOP("App::Property","Camera position data associated with this object"))
            vobj.setEditorMode("ViewData",2)
        if not "RestoreView" in pl:
            vobj.addProperty("App::PropertyBool","RestoreView","Interaction",QT_TRANSLATE_NOOP("App::Property","If set, the view stored in this object will be restored on double-click"))
        if not "DoubleClickActivates" in pl:
            vobj.addProperty("App::PropertyBool","DoubleClickActivates","Interaction",QT_TRANSLATE_NOOP("App::Property","If True, double-clicking this object in the tree turns it active"))

        # inventor saving
        if not "SaveInventor" in pl:
            vobj.addProperty("App::PropertyBool","SaveInventor","Interaction",QT_TRANSLATE_NOOP("App::Property","If this is enabled, the inventor representation of this object will be saved in the FreeCAD file, allowing to reference it in other file sin lightweight mode."))
        if not "SavedInventor" in pl:
            vobj.addProperty("App::PropertyFileIncluded","SavedInventor","Interaction",QT_TRANSLATE_NOOP("App::Property","A slot to save the inventor representation of this object, if enabled"))
            vobj.setEditorMode("SavedInventor",2)

        # children properties
        if not "ChildrenOverride" in pl:
            vobj.addProperty("App::PropertyBool","ChildrenOverride","Children",QT_TRANSLATE_NOOP("App::Property","If true, show the objects contained in this Building Part will adopt these line, color and transparency settings"))
        if not "ChildrenLineWidth" in pl:
            vobj.addProperty("App::PropertyFloat","ChildrenLineWidth","Children",QT_TRANSLATE_NOOP("App::Property","The line width of child objects"))
            vobj.LineWidth = 1
        if not "ChildrenLineColor" in pl:
            vobj.addProperty("App::PropertyColor","ChildrenLineColor","Children",QT_TRANSLATE_NOOP("App::Property","The line color of child objects"))
            c = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned("DefaultShapeLineColor",255)
            vobj.ChildrenLineColor = (float((c>>24)&0xFF)/255.0,float((c>>16)&0xFF)/255.0,float((c>>8)&0xFF)/255.0,0.0)
        if not "ChildrenShapeolor" in pl:
            vobj.addProperty("App::PropertyColor","ChildrenShapeColor","Children",QT_TRANSLATE_NOOP("App::Property","The shape color of child objects"))
            c = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned("DefaultShapeColor",4294967295)
            vobj.ChildrenLineColor = (float((c>>24)&0xFF)/255.0,float((c>>16)&0xFF)/255.0,float((c>>8)&0xFF)/255.0,0.0)
        if not "ChildrenTransparency" in pl:
            vobj.addProperty("App::PropertyPercent","ChildrenTransparency","Children",QT_TRANSLATE_NOOP("App::Property","The transparency of child objects"))

        # clip properties
        if not "CutView" in pl:
            vobj.addProperty("App::PropertyBool","CutView","Clip",QT_TRANSLATE_NOOP("App::Property","Cut the view above this level"))
        if not "CutMargin" in pl:
            vobj.addProperty("App::PropertyLength","CutMargin","Clip",QT_TRANSLATE_NOOP("App::Property","The distance between the level plane and the cut line"))
            vobj.CutMargin = 1600
        if not "AutoCutView" in pl:
            vobj.addProperty("App::PropertyBool","AutoCutView","Clip",QT_TRANSLATE_NOOP("App::Property","Turn cutting on when activating this level"))

    def onDocumentRestored(self,vobj):

        selt.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if self.Object.IfcType == "Building Storey":
                return ":/icons/Arch_Floor_Tree.svg"
            elif self.Object.IfcType == "Building":
                return ":/icons/Arch_Building_Tree.svg"
        return ":/icons/Arch_BuildingPart_Tree.svg"

    def attach(self,vobj):

        self.Object = vobj.Object
        self.clip = None
        from pivy import coin
        self.sep = coin.SoGroup()
        self.mat = coin.SoMaterial()
        self.sep.addChild(self.mat)
        self.dst = coin.SoDrawStyle()
        self.sep.addChild(self.dst)
        self.lco = coin.SoCoordinate3()
        self.sep.addChild(self.lco)
        lin = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        lin.coordIndex.setValues([0,1,-1,2,3,-1,4,5,-1])
        self.sep.addChild(lin)
        self.tra = coin.SoTransform()
        self.tra.rotation.setValue(FreeCAD.Rotation(0,0,90).Q)
        self.sep.addChild(self.tra)
        self.fon = coin.SoFont()
        self.sep.addChild(self.fon)
        self.txt = coin.SoAsciiText()
        self.txt.justification = coin.SoText2.LEFT
        self.txt.string.setValue("level")
        self.sep.addChild(self.txt)
        vobj.addDisplayMode(self.sep,"Default")
        self.onChanged(vobj,"ShapeColor")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ShowLevel")
        self.onChanged(vobj,"FontSize")
        return

    def getDisplayModes(self,vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self,mode):

        return mode

    def updateData(self,obj,prop):

        if prop in ["Placement","LevelOffset"]:
            self.onChanged(obj.ViewObject,"OverrideUnit")
        elif prop == "Shape":
            # gather all the child shapes
            colors = self.getColors(obj)
            if colors and hasattr(obj.ViewObject,"DiffuseColor"):
                if len(colors) == len(obj.Shape.Faces):
                    if colors != obj.ViewObject.DiffuseColor:
                        obj.ViewObject.DiffuseColor = colors
                        self.writeInventor(obj)
                #else:
                    #print("color mismatch:",len(colors),"colors,",len(obj.Shape.Faces),"faces")
        elif prop == "Group":
            self.onChanged(obj.ViewObject,"ChildrenOverride")
        elif prop == "Label":
            self.onChanged(obj.ViewObject,"ShowLabel")

    def getColors(self,obj):

        "recursively get the colors of objects inside this BuildingPart"

        colors = []
        for child in Draft.getGroupContents(obj):
            if hasattr(child,'Shape'):
                if len(child.ViewObject.DiffuseColor) == len(child.Shape.Faces):
                    colors.extend(child.ViewObject.DiffuseColor)
                else:
                    c = child.ViewObject.ShapeColor[:3]+(child.ViewObject.Transparency/100.0,)
                    for i in range(len(child.Shape.Faces)):
                        colors.append(c)
        return colors

    def onChanged(self,vobj,prop):

        #print(vobj.Object.Label," - ",prop)

        if prop == "ShapeColor":
            if hasattr(vobj,"ShapeColor"):
                l = vobj.ShapeColor
                self.mat.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "LineWidth":
            if hasattr(vobj,"LineWidth"):
                self.dst.lineWidth = vobj.LineWidth
        elif prop == "FontName":
            if hasattr(vobj,"FontName"):
                if vobj.FontName:
                    if sys.version_info.major < 3:
                        self.fon.name = vobj.FontName.encode("utf8")
                    else:
                        self.fon.name = vobj.FontName
        elif prop in ["FontSize","DisplayOffset","OriginOffset"]:
            if hasattr(vobj,"FontSize") and hasattr(vobj,"DisplayOffset") and hasattr(vobj,"OriginOffset"):
                fs = vobj.FontSize.Value
                if fs:
                    self.fon.size = fs
                    b = vobj.DisplayOffset.Base
                    self.tra.translation.setValue([b.x+fs/8,b.y,b.z+fs/8])
                    r = vobj.DisplayOffset.Rotation
                    self.tra.rotation.setValue(r.Q)
                    if vobj.OriginOffset:
                        self.lco.point.setValues([[b.x-fs,b.y,b.z],[b.x+fs,b.y,b.z],[b.x,b.y-fs,b.z],[b.x,b.y+fs,b.z],[b.x,b.y,b.z-fs],[b.x,b.y,b.z+fs]])
                    else:
                        self.lco.point.setValues([[-fs,0,0],[fs,0,0],[0,-fs,0],[0,fs,0],[0,0,-fs],[0,0,fs]])
        elif prop in ["OverrideUnit","ShowUnit","ShowLevel","ShowLabel"]:
            if hasattr(vobj,"OverrideUnit") and hasattr(vobj,"ShowUnit") and hasattr(vobj,"ShowLevel") and hasattr(vobj,"ShowLabel"):
                z = vobj.Object.Placement.Base.z + vobj.Object.LevelOffset.Value
                q = FreeCAD.Units.Quantity(z,FreeCAD.Units.Length)
                txt = ""
                if vobj.ShowLabel:
                    txt += vobj.Object.Label
                if vobj.ShowLevel:
                    if txt:
                        txt += " "
                    if z >= 0:
                        txt += "+"
                    if vobj.OverrideUnit:
                        u = vobj.OverrideUnit
                    else:
                        u = q.getUserPreferred()[2]
                    try:
                        q = q.getValueAs(u)
                    except:
                        q = q.getValueAs(q.getUserPreferred()[2])
                    d = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",0)
                    fmt = "{0:."+ str(d) + "f}"
                    if not vobj.ShowUnit:
                        u = ""
                    txt += fmt.format(float(q)) + str(u)
                if not txt:
                    txt = " " # empty texts make coin crash...
                if isinstance(txt,unicode):
                    txt = txt.encode("utf8")
                self.txt.string.setValue(txt)
        elif prop in ["ChildrenOverride","ChildenLineWidth","ChildrenLineColor","ChildrenShapeColor","ChildrenTransparency"]:
            if hasattr(vobj,"ChildrenOverride") and vobj.ChildrenOverride:
                props = ["ChildenLineWidth","ChildrenLineColor","ChildrenShapeColor","ChildrenTransparency"]
                for child in vobj.Object.Group:
                    for prop in props:
                        if hasattr(vobj,prop) and hasattr(child.ViewObject,prop[8:]) and not hasattr(child,"ChildrenOverride"):
                            setattr(child.ViewObject,prop[8:],getattr(vobj,prop))
        elif prop in ["CutView","CutMargin"]:
            if hasattr(vobj,"CutView") and FreeCADGui.ActiveDocument.ActiveView:
                sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
                if vobj.CutView:
                    from pivy import coin
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.getGroupContents(vobj.Object.Group,walls=True):
                        if hasattr(o.ViewObject,"Lighting"):
                            o.ViewObject.Lighting = "One side"
                    self.clip = coin.SoClipPlane()
                    self.clip.on.setValue(True)
                    norm = vobj.Object.Placement.multVec(FreeCAD.Vector(0,0,1))
                    mp = vobj.Object.Placement.Base
                    mp = DraftVecUtils.project(mp,norm)
                    dist = mp.Length #- 0.1 # to not clip exactly on the section object
                    norm = norm.negative()
                    marg = 1
                    if hasattr(vobj,"CutMargin"):
                        marg = vobj.CutMargin.Value
                    if mp.getAngle(norm) > 1:
                        dist += marg
                        dist = -dist
                    else:
                        dist -= marg
                    plane = coin.SbPlane(coin.SbVec3f(norm.x,norm.y,norm.z),dist)
                    self.clip.plane.setValue(plane)
                    sg.insertChild(self.clip,0)
                else:
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.getGroupContents(vobj.Object.Group,walls=True):
                        if hasattr(o.ViewObject,"Lighting"):
                            o.ViewObject.Lighting = "Two side"
        elif prop == "Visibility":
            # turn clipping off when turning the object off
            if hasattr(vobj,"Visibility") and not(vobj.Visibility) and hasattr(vobj,"CutView"):
                vobj.CutView = False
        elif prop == "SaveInventor":
            self.writeInventor(vobj.Object)

    def onDelete(self,vobj,subelements):

        if self.clip:
            sg.removeChild(self.clip)
            self.clip = None
        for o in Draft.getGroupContents(vobj.Object.Group,walls=True):
            if hasattr(o.ViewObject,"Lighting"):
                o.ViewObject.Lighting = "Two side"
        return True

    def doubleClicked(self,vobj):

        self.activate(vobj)
        if (not hasattr(vobj,"DoubleClickActivates")) or vobj.DoubleClickActivates:
            FreeCADGui.Selection.clearSelection()
        return True

    def activate(self,vobj):

        if FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch") == vobj.Object:
            FreeCADGui.ActiveDocument.ActiveView.setActiveObject("Arch",None)
            if vobj.SetWorkingPlane:
                self.setWorkingPlane(restore=True)
        else:
            if (not hasattr(vobj,"DoubleClickActivates")) or vobj.DoubleClickActivates:
                FreeCADGui.ActiveDocument.ActiveView.setActiveObject("Arch",vobj.Object)
            if vobj.SetWorkingPlane:
                self.setWorkingPlane()

    def setupContextMenu(self,vobj,menu):

        from PySide import QtCore,QtGui
        import Draft_rc
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Set working plane",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.setWorkingPlane)
        menu.addAction(action1)
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Write camera position",menu)
        QtCore.QObject.connect(action2,QtCore.SIGNAL("triggered()"),self.writeCamera)
        menu.addAction(action2)
        action3 = QtGui.QAction(QtGui.QIcon(),"Create group...",menu)
        QtCore.QObject.connect(action3,QtCore.SIGNAL("triggered()"),self.createGroup)
        menu.addAction(action3)
        action4 = QtGui.QAction(QtGui.QIcon(),"Reorder children alphabetically",menu)
        QtCore.QObject.connect(action4,QtCore.SIGNAL("triggered()"),self.reorder)
        menu.addAction(action4)
        action5 = QtGui.QAction(QtGui.QIcon(),"Clone level up",menu)
        QtCore.QObject.connect(action5,QtCore.SIGNAL("triggered()"),self.cloneUp)
        menu.addAction(action5)

    def setWorkingPlane(self,restore=False):

        if hasattr(self,"Object") and hasattr(FreeCAD,"DraftWorkingPlane"):
            import FreeCADGui
            autoclip = False
            if hasattr(self.Object.ViewObject,"AutoCutView"):
                autoclip = self.Object.ViewObject.AutoCutView
            if restore:
                FreeCAD.DraftWorkingPlane.restore()
                if autoclip:
                    self.Object.ViewObject.CutView = False
            else:
                FreeCAD.DraftWorkingPlane.save()
                FreeCADGui.runCommand("Draft_SelectPlane")
                if autoclip:
                    self.Object.ViewObject.CutView = True
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                if restore and hasattr(self,"wptext"):
                    FreeCADGui.draftToolBar.wplabel.setText(self.wptext)
                else:
                    self.wptext = FreeCADGui.draftToolBar.wplabel.text()
                    FreeCADGui.draftToolBar.wplabel.setText(self.Object.Label)

    def writeCamera(self):

        if hasattr(self,"Object"):
            from pivy import coin
            n = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            FreeCAD.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing camera position")+"\n")
            cdata = list(n.position.getValue().getValue())
            cdata.extend(list(n.orientation.getValue().getValue()))
            cdata.append(n.nearDistance.getValue())
            cdata.append(n.farDistance.getValue())
            cdata.append(n.aspectRatio.getValue())
            cdata.append(n.focalDistance.getValue())
            if isinstance(n,coin.SoOrthographicCamera):
                cdata.append(n.height.getValue())
                cdata.append(0.0) # orthograhic camera
            elif isinstance(n,coin.SoPerspectiveCamera):
                cdata.append(n.heightAngle.getValue())
                cdata.append(1.0) # perspective camera
            self.Object.ViewObject.ViewData = cdata

    def createGroup(self):

        if hasattr(self,"Object"):
            s = "FreeCAD.ActiveDocument.getObject(\"%s\").newObject(\"App::DocumentObjectGroup\",\"Group\")" % self.Object.Name
            FreeCADGui.doCommand(s)

    def reorder(self):

        if hasattr(self,"Object"):
            if hasattr(self.Object,"Group") and self.Object.Group:
                g = self.Object.Group
                g.sort(key=lambda obj: obj.Label)
                self.Object.Group = g
                FreeCAD.ActiveDocument.recompute()

    def cloneUp(self):

        if hasattr(self,"Object"):
            if not self.Object.Height.Value:
                FreeCAD.Console.PrintError("This level has no height value. Please define a height before using this function.\n")
                return
            height = self.Object.Height.Value
            ng = []
            if hasattr(self.Object,"Group") and self.Object.Group:
                for o in self.Object.Group:
                    no = Draft.clone(o)
                    Draft.move(no,FreeCAD.Vector(0,0,height))
                    ng.append(no)
            nobj = makeBuildingPart()
            Draft.formatObject(nobj,self.Object)
            nobj.Placement = self.Object.Placement
            nobj.Placement.move(FreeCAD.Vector(0,0,height))
            nobj.IfcType = self.Object.IfcType
            nobj.Height = height
            nobj.Label = self.Object.Label
            nobj.Group = ng
            for parent in self.Object.InList:
                if hasattr(parent,"Group") and hasattr(parent,"addObject") and (self.Object in parent.Group):
                    parent.addObject(nobj)
            FreeCAD.ActiveDocument.recompute()
            # fix for missing IFC attributes
            for no in ng:
                if hasattr(no,"LongName") and hasattr(no,"CloneOf") and no.CloneOf and hasattr(no.CloneOf,"LongName"):
                    no.LongName = no.CloneOf.LongName
            FreeCAD.ActiveDocument.recompute()

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def writeInventor(self,obj):

        def callback(match):
            return next(callback.v)

        if hasattr(obj.ViewObject,"SaveInventor") and obj.ViewObject.SaveInventor:
            if obj.Shape and obj.Shape.Faces and hasattr(obj,"SavedInventor"):
                colors = obj.ViewObject.DiffuseColor
                if len(colors) != len(obj.Shape.Faces):
                    print("Debug: Colors mismatch in",obj.Label)
                    colors = None
                iv = self.Object.Shape.writeInventor()
                import re
                if colors:
                    if len(re.findall("IndexedFaceSet",iv)) == len(obj.Shape.Faces):
                        # convert colors to iv representations
                        colors = ["Material { diffuseColor "+str(color[0])+" "+str(color[1])+" "+str(color[2])+"}\n    IndexedFaceSet" for color in colors]
                        # replace
                        callback.v=iter(colors)
                        iv = re.sub("IndexedFaceSet",callback,iv)
                    else:
                        print("Debug: IndexedFaceSet mismatch in",obj.Label)
                # save embedded file
                tf = tempfile.mkstemp(prefix=obj.Name,suffix=".iv")[1]
                f = open(tf,"w")
                f.write(iv)
                f.close()
                obj.SavedInventor = tf
                os.remove(tf)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_BuildingPart',CommandBuildingPart())
