/***************************************************************************
 *   Copyright (c) 2013-2014 Luke Parry <l.parry@warwick.ac.uk>            *
 *   Copyright (c) 2014 Joe Dowsett <dowsettjoe[at]yahoo[dot]co[dot]uk>    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#include <QRectF>
#include <cmath>
#endif

#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax2.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"
#include "DrawPage.h"
#include "DrawProjGroupItem.h"
#include "DrawProjGroup.h"

#include <Mod/TechDraw/App/DrawProjGroupPy.h>  // generated from DrawProjGroupPy.xml

using namespace TechDraw;

const char* DrawProjGroup::ProjectionTypeEnums[] = {"Default",
                                                              "First Angle",
                                                              "Third Angle",
                                                              NULL};



PROPERTY_SOURCE(TechDraw::DrawProjGroup, TechDraw::DrawViewCollection)

DrawProjGroup::DrawProjGroup(void)
{
    static const char *group = "Base";
    static const char *agroup = "Distribute";

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                               GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool autoDist = hGrp->GetBool("AutoDist",true);


    ADD_PROPERTY_TYPE(Source    ,(0), group, App::Prop_None,"Shape to view");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(Anchor, (0), group, App::Prop_None, "The root view to align projections with");
    Anchor.setScope(App::LinkScope::Global);


    ProjectionType.setEnums(ProjectionTypeEnums);
    ADD_PROPERTY_TYPE(ProjectionType, ((long)getDefProjConv()), group,
                                App::Prop_None, "First or Third Angle projection");

    ADD_PROPERTY_TYPE(AutoDistribute ,(autoDist),agroup,
                                App::Prop_None,"Distribute Views Automatically or Manually");
    ADD_PROPERTY_TYPE(spacingX, (15), agroup, App::Prop_None, "Horizontal spacing between views");
    ADD_PROPERTY_TYPE(spacingY, (15), agroup, App::Prop_None, "Vertical spacing between views");
    Rotation.setStatus(App::Property::Hidden,true);   //DPG does not rotate
    Caption.setStatus(App::Property::Hidden,true);

}

DrawProjGroup::~DrawProjGroup()
{
}

void DrawProjGroup::onChanged(const App::Property* prop)
{
    //TODO: For some reason, when the projection type is changed, the isometric views show change appropriately, but the orthographic ones don't... Or vice-versa.  WF: why would you change from 1st to 3rd in mid drawing?
    //if group hasn't been added to page yet, can't scale or distribute projItems
    TechDraw::DrawPage *page = getPage();
    if (!isRestoring() && page) {
        if (prop == &Source) {
//            std::vector<App::DocumentObject*> sourceObjs = Source.getValues();
//            if (!sourceObjs.empty()) {
//                if (!hasAnchor()) {
//                    // if we have a Source, but no Anchor, make an anchor
//                    Anchor.setValue(addProjection("Front"));      //<<<<< semi-loop here!
//                    //add projection marks object as changed -> onChanged, but anchor value isn't set
//                    Anchor.purgeTouched();                      //don't need to mark this
//                }
//            } else {
//                //Source has been changed to null! Why? What to do?
//            }
        }
        if (prop == &Scale) {
            updateChildrenScale();
        }

        if (prop == &ProjectionType) {
            updateChildrenEnforce();
        }

        if (prop == &Source) {
            updateChildrenSource();
        }

        if (prop == &LockPosition) {
            updateChildrenLock();
        }

        if (prop == &ScaleType) {
            double newScale = getScale();
            if (ScaleType.isValue("Automatic")) {
                //Recalculate scale if Group is too big or too small!
                newScale = calculateAutomaticScale();
                if(std::abs(getScale() - newScale) > FLT_EPSILON) {
                    Scale.setValue(newScale);
                }
            } else if (ScaleType.isValue("Page")) {
                newScale = page->Scale.getValue();
                if(std::abs(getScale() - newScale) > FLT_EPSILON) {
                    Scale.setValue(newScale);
                }
            }
        }
        if (prop == &Rotation) {
            if (!DrawUtil::fpCompare(Rotation.getValue(),0.0)) {
                Rotation.setValue(0.0);
                purgeTouched();
                Base::Console().Log("DPG: Projection Groups do not rotate. Change ignored.\n");
            }
        }
    }

    TechDraw::DrawViewCollection::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawProjGroup::execute(void)
{
//    Base::Console().Message("DPG::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    //if group hasn't been added to page yet, can't scale or distribute projItems
    TechDraw::DrawPage *page = getPage();
    if (!page) {
        return DrawViewCollection::execute();
    }

    std::vector<App::DocumentObject*> docObjs = Source.getValues();
    if (docObjs.empty()) {
        return DrawViewCollection::execute();
    }

    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj == nullptr) {
        //no anchor yet.  nothing to do.
        return DrawViewCollection::execute();
    }

    autoPositionChildren();

    return DrawViewCollection::execute();
}

short DrawProjGroup::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result = Views.isTouched() ||
                 Source.isTouched() ||
                 Scale.isTouched()  ||
                 ScaleType.isTouched() ||
                 ProjectionType.isTouched() ||
                 Anchor.isTouched() ||
                 AutoDistribute.isTouched() ||
                 LockPosition.isTouched()||
                 spacingX.isTouched() ||
                 spacingY.isTouched();
    }
    if (result) return result;
    return TechDraw::DrawViewCollection::mustExecute();
}

Base::BoundBox3d DrawProjGroup::getBoundingBox() const
{
    Base::BoundBox3d bbox;

    std::vector<App::DocumentObject*> views = Views.getValues();
    TechDraw::DrawProjGroupItem *anchorView = dynamic_cast<TechDraw::DrawProjGroupItem *>(Anchor.getValue());
    if (anchorView == nullptr) {
        //if an element in Views is not a DPGI, something really bad has happened somewhere
        Base::Console().Log("PROBLEM - DPG::getBoundingBox - non DPGI entry in Views! %s\n",
                                getNameInDocument());
        throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
    }
    for (std::vector<App::DocumentObject*>::const_iterator it = views.begin(); it != views.end(); ++it) {
         if ((*it)->getTypeId().isDerivedFrom(DrawViewPart::getClassTypeId())) {
            DrawViewPart *part = static_cast<DrawViewPart *>(*it);
            Base::BoundBox3d  bb = part->getBoundingBox();

            bb.ScaleX(1. / part->getScale());
            bb.ScaleY(1. / part->getScale());

            // X and Y of dependent views are relative to the anchorView
            if (part != anchorView) {
                bb.MoveX(part->X.getValue());
                bb.MoveY(part->Y.getValue());
            }

            bbox.Add(bb);
        }
    }
    return bbox;
}

TechDraw::DrawPage * DrawProjGroup::getPage(void) const
{
    return findParentPage();
}

// Function provided by Joe Dowsett, 2014
double DrawProjGroup::calculateAutomaticScale() const
{
    TechDraw::DrawPage *page = getPage();
    if (page == NULL)
      throw Base::RuntimeError("No page is assigned to this feature");

    DrawProjGroupItem *viewPtrs[10];

    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);                               //get 1:1 bbxs
                                            // if Page.keepUpdated is false, and DrawViews have never been executed,
                                            // bb's will be 0x0 and this routine will return 0!!!
                                            // if we return 1.0, AutoScale will sort itself out once bb's are non-zero.
    double bbFudge = 1.2;
    width *= bbFudge;
    height *= bbFudge;


    // C++ Standard says casting bool to int gives 0 or 1
    int numVertSpaces = (viewPtrs[0] || viewPtrs[3] || viewPtrs[7]) +
                        (viewPtrs[2] || viewPtrs[5] || viewPtrs[9]) +
                        (viewPtrs[6] != NULL);
    int numHorizSpaces = (viewPtrs[0] || viewPtrs[1] || viewPtrs[2]) +
                         (viewPtrs[7] || viewPtrs[8] || viewPtrs[9]);

    double availableX = page->getPageWidth();
    double availableY = page->getPageHeight();
    double xWhite = spacingX.getValue() * (numVertSpaces + 1);
    double yWhite = spacingY.getValue() * (numHorizSpaces + 1);
    width += xWhite;
    height += yWhite;
    double scale_x = availableX / width;
    double scale_y = availableY / height;

    double scaleFudge = 0.80;
    float working_scale = scaleFudge * std::min(scale_x, scale_y);
    double result = DrawUtil::sensibleScale(working_scale);
    if (!(result > 0.0)) {
        Base::Console().Log("DPG - %s - bad scale found (%.3f) using 1.0\n",getNameInDocument(),result);
        result = 1.0;
    }

    return result;
}

QRectF DrawProjGroup::getRect() const         //this is current rect, not potential rect
{
    DrawProjGroupItem *viewPtrs[10];
    arrangeViewPointers(viewPtrs);
    double width, height;
    minimumBbViews(viewPtrs, width, height);                           // w,h of just the views at 1:1 scale
    double xSpace = spacingX.getValue() * 3.0 * std::max(1.0,getScale());
    double ySpace = spacingY.getValue() * 2.0 * std::max(1.0,getScale());
    double rectW = getScale() * width + xSpace;                  //scale the 1:1 w,h and add whitespace
    double rectH = getScale() * height + ySpace;
    return QRectF(0,0,rectW,rectH);
}

//find area consumed by Views only in 1:1 Scale
void DrawProjGroup::minimumBbViews(DrawProjGroupItem *viewPtrs[10],
                                            double &width, double &height) const
{
    // Get bounding boxes in object scale
    Base::BoundBox3d bboxes[10];
    makeViewBbs(viewPtrs, bboxes, true);

    //TODO: note that TLF/TRF/BLF,BRF extend a bit farther than a strict row/col arrangement would suggest.
    //get widest view in each row/column
    double col0w = std::max(std::max(bboxes[0].LengthX(), bboxes[3].LengthX()), bboxes[7].LengthX()),
           col1w = std::max(std::max(bboxes[1].LengthX(), bboxes[4].LengthX()), bboxes[8].LengthX()),
           col2w = std::max(std::max(bboxes[2].LengthX(), bboxes[5].LengthX()), bboxes[9].LengthX()),
           col3w = bboxes[6].LengthX(),
           row0h = std::max(std::max(bboxes[0].LengthY(), bboxes[1].LengthY()), bboxes[2].LengthY()),
           row1h = std::max(std::max(bboxes[3].LengthY(), bboxes[4].LengthY()),
                            std::max(bboxes[5].LengthY(), bboxes[6].LengthY())),
           row2h = std::max(std::max(bboxes[7].LengthY(), bboxes[8].LengthY()), bboxes[9].LengthY());

    width = col0w + col1w + col2w + col3w;
    height = row0h + row1h + row2h;
}

App::DocumentObject * DrawProjGroup::getProjObj(const char *viewProjType) const
{
    for( auto it : Views.getValues() ) {
        auto projPtr( dynamic_cast<DrawProjGroupItem *>(it) );
        if (projPtr == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::getProjObj - non DPGI entry in Views! %s / %s\n",
                                    getNameInDocument(),viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else if(strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
            return it;
        }
    }

    return 0;
}

DrawProjGroupItem* DrawProjGroup::getProjItem(const char *viewProjType) const
{
    App::DocumentObject* docObj = getProjObj(viewProjType);
    auto result( dynamic_cast<TechDraw::DrawProjGroupItem *>(docObj) );
    if ( (result == nullptr) &&
         (docObj != nullptr) ) {
        //should never have a item in DPG that is not a DPGI.
        Base::Console().Log("PROBLEM - DPG::getProjItem finds non-DPGI in Group %s / %s\n",
                                getNameInDocument(),viewProjType);
        throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
    }
    return result;
}

bool DrawProjGroup::checkViewProjType(const char *in)
{
    if ( strcmp(in, "Front") == 0 ||
         strcmp(in, "Left") == 0 ||
         strcmp(in, "Right") == 0 ||
         strcmp(in, "Top") == 0 ||
         strcmp(in, "Bottom") == 0 ||
         strcmp(in, "Rear") == 0 ||
         strcmp(in, "FrontTopLeft") == 0 ||
         strcmp(in, "FrontTopRight") == 0 ||
         strcmp(in, "FrontBottomLeft") == 0 ||
         strcmp(in, "FrontBottomRight") == 0) {
        return true;
    }
    return false;
}

//********************************
// ProjectionItem A/D/I
//********************************
bool DrawProjGroup::hasProjection(const char *viewProjType) const
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<TechDraw::DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //should never have a item in DPG that is not a DPGI.
            Base::Console().Log("PROBLEM - DPG::hasProjection finds non-DPGI in Group %s / %s\n",
                                    getNameInDocument(),viewProjType);
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }

        if (strcmp(viewProjType, view->Type.getValueAsString()) == 0 ) {
            return true;
        }
    }
    return false;
}

App::DocumentObject * DrawProjGroup::addProjection(const char *viewProjType)
{
    DrawProjGroupItem *view( nullptr );
    std::pair<Base::Vector3d,Base::Vector3d> vecs;

    DrawPage* dp = findParentPage();
    if (dp == nullptr) {
        Base::Console().Error("DPG:addProjection - %s - DPG is not on a page!\n",getNameInDocument());
    }

    if ( checkViewProjType(viewProjType) && !hasProjection(viewProjType) ) {
        std::string FeatName = getDocument()->getUniqueObjectName("ProjItem");
        auto docObj( getDocument()->addObject( "TechDraw::DrawProjGroupItem",     //add to Document
                                               FeatName.c_str() ) );
        view = dynamic_cast<TechDraw::DrawProjGroupItem *>(docObj);
        if ( (view == nullptr) &&
             (docObj != nullptr) ) {
            //should never happen that we create a DPGI that isn't a DPGI!!
            Base::Console().Log("PROBLEM - DPG::addProjection - created a non DPGI! %s / %s\n",
                                    getNameInDocument(),viewProjType);
            throw Base::TypeError("Error: new projection is not a DPGI!");
        }
        if (view != nullptr) {                        //coverity CID 151722
            addView(view);                            //from DrawViewCollection
            view->Source.setValues( Source.getValues() );
            view->Scale.setValue( getScale() );
            view->Type.setValue( viewProjType );
            view->Label.setValue( viewProjType );
            view->Source.setValues( Source.getValues() );
            if (strcmp(viewProjType, "Front") != 0 ) {  //not Front!
                vecs = getDirsFromFront(view);
                view->Direction.setValue(vecs.first);
                view->XDirection.setValue(vecs.second);
                view->recomputeFeature();
            } else {  //Front
                Anchor.setValue(view);
                Anchor.purgeTouched();
                view->LockPosition.setValue(true);  //lock "Front" position within DPG (note not Page!).
                view->LockPosition.setStatus(App::Property::ReadOnly,true); //Front should stay locked.
                view->LockPosition.purgeTouched();
                requestPaint();   //make sure the group object is on the Gui page
            }
        //        addView(view);                            //from DrawViewCollection
        //        if (view != getAnchor()) {                //anchor is done elsewhere
        //            view->recomputeFeature();
        //        }
        }
    }
    return view;
}

//NOTE: projections can be deleted without using removeProjection - ie regular DocObject deletion process.
int DrawProjGroup::removeProjection(const char *viewProjType)
{
    // TODO: shouldn't be able to delete "Front" unless deleting whole group
    if ( checkViewProjType(viewProjType) ) {
        if( !hasProjection(viewProjType) ) {
            throw Base::RuntimeError("The projection doesn't exist in the group");
        }

        // Iterate through the child views and find the projection type
        for( auto it : Views.getValues() ) {
            auto projPtr( dynamic_cast<TechDraw::DrawProjGroupItem *>(it) );
            if( projPtr != nullptr) {
                if ( strcmp(viewProjType, projPtr->Type.getValueAsString()) == 0 ) {
                    removeView(projPtr);                                           // Remove from collection
                    getDocument()->removeObject( it->getNameInDocument() );        // Remove from the document
                    return Views.getValues().size();
                }
            } else {
                //if an element in Views is not a DPGI, something really bad has happened somewhere
                Base::Console().Log("PROBLEM - DPG::removeProjection - tries to remove non DPGI! %s / %s\n",
                                    getNameInDocument(),viewProjType);
                throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
            }
        }
    }

    return -1;
}

//removes all DPGI - used when deleting DPG
int DrawProjGroup::purgeProjections()
{
    while (!Views.getValues().empty())   {
        std::vector<DocumentObject*> views = Views.getValues();
        DrawProjGroupItem* dpgi;
        DocumentObject* dObj =  views.back();
        dpgi = dynamic_cast<DrawProjGroupItem*>(dObj);
        if (dpgi != nullptr) {
            std::string itemName = dpgi->Type.getValueAsString();
            removeProjection(itemName.c_str());
        } else {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::purgeProjection - tries to remove non DPGI! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
    }
    auto page = findParentPage();
    if (page != nullptr) {
        page->requestPaint();
    }

    return Views.getValues().size();
}

std::pair<Base::Vector3d,Base::Vector3d> DrawProjGroup::getDirsFromFront(DrawProjGroupItem* view)
{
    std::pair<Base::Vector3d,Base::Vector3d> result;
    std::string viewType = view->Type.getValueAsString();
    result = getDirsFromFront(viewType);
    return result;
}

std::pair<Base::Vector3d,Base::Vector3d> DrawProjGroup::getDirsFromFront(std::string viewType)
{
//    Base::Console().Message("DPG::getDirsFromFront(%s)\n", viewType.c_str());
    std::pair<Base::Vector3d,Base::Vector3d> result;

    Base::Vector3d projDir, rotVec;
    DrawProjGroupItem* anch = getAnchor();
    if (anch == nullptr) {
        Base::Console().Warning("DPG::getDirsFromFront - %s - No Anchor!\n",Label.getValue());
        throw Base::RuntimeError("Project Group missing Anchor projection item");
    }

    Base::Vector3d dirAnch = anch->Direction.getValue();
    Base::Vector3d rotAnch = anch->getXDirection();
    result = std::make_pair(dirAnch,rotAnch);

    Base::Vector3d org(0.0,0.0,0.0);
    gp_Ax2 anchorCS = anch->getProjectionCS(org);
    gp_Pnt gOrg(0.0, 0.0, 0.0);
    gp_Dir gDir = anchorCS.Direction();
    gp_Dir gXDir = anchorCS.XDirection();
    gp_Dir gYDir = anchorCS.YDirection();
    gp_Ax1 gUpAxis(gOrg, gYDir);
    gp_Ax2 newCS;
    gp_Dir gNewDir;
    gp_Dir gNewXDir;
    
    double angle = M_PI / 2.0;                        //90*

    if (viewType == "Right") {
        newCS = anchorCS.Rotated(gUpAxis, angle);
        projDir = dir2vec(newCS.Direction());
        rotVec  = dir2vec(newCS.XDirection());
    } else if (viewType == "Left") {
        newCS = anchorCS.Rotated(gUpAxis, -angle);
        projDir = dir2vec(newCS.Direction());
        rotVec  = dir2vec(newCS.XDirection());
    } else if (viewType == "Top") {
        projDir = dir2vec(gYDir);
        rotVec  = dir2vec(gXDir);
    } else if (viewType == "Bottom") {
        projDir = dir2vec(gYDir.Reversed());
        rotVec  = dir2vec(gXDir);
    } else if (viewType == "Rear") {
        projDir = dir2vec(gDir.Reversed());
        rotVec  = dir2vec(gXDir.Reversed());
    } else if (viewType == "FrontTopLeft") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) -
                               gp_Vec(gXDir) +
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) +
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else if (viewType == "FrontTopRight") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) +
                               gp_Vec(gXDir) +
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) -
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else if (viewType == "FrontBottomLeft") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) -
                               gp_Vec(gXDir) -
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) +
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else if (viewType == "FrontBottomRight") {
        gp_Dir newDir = gp_Dir(gp_Vec(gDir) +
                               gp_Vec(gXDir) -
                               gp_Vec(gYDir));
        projDir = dir2vec(newDir);
        gp_Dir newXDir = gp_Dir(gp_Vec(gXDir) -
                                gp_Vec(gDir));
        rotVec = dir2vec(newXDir);
    } else {
        Base::Console().Error("DrawProjGroup - %s unknown projection: %s\n",getNameInDocument(),viewType.c_str());
        return result;
    }

    result = std::make_pair(projDir,rotVec);
    return result;
}

Base::Vector3d DrawProjGroup::dir2vec(gp_Dir d)
{
    Base::Vector3d result(d.X(),
                        d.Y(),
                        d.Z());
    return result;
}

gp_Dir DrawProjGroup::vec2dir(Base::Vector3d v)
{
    gp_Dir result(v.x,
                  v.y,
                  v.z);
    return result;
}

Base::Vector3d DrawProjGroup::getXYPosition(const char *viewTypeCStr)
{
    Base::Vector3d result(0.0,0.0,0.0);
    //Front view position is always (0,0)
    if (strcmp(viewTypeCStr, "Front") == 0 ) {  // Front!
        return result;
    }
    const int idxCount = 10;
    DrawProjGroupItem *viewPtrs[idxCount];
    arrangeViewPointers(viewPtrs);
    int viewIndex = getViewIndex(viewTypeCStr);

        //TODO: bounding boxes do not take view orientation into account
        //      ie X&Y widths might be swapped on page

//    if (AutoDistribute.getValue()) {
    if (true) {
        std::vector<Base::Vector3d> position(idxCount);
        int idx = 0;
        for (;idx < idxCount; idx++) {
            if (viewPtrs[idx]) {
                position[idx].x = viewPtrs[idx]->X.getValue();
                position[idx].y = viewPtrs[idx]->Y.getValue();
            }
        }

        // Calculate bounding boxes for each displayed view
        Base::BoundBox3d bboxes[10];
        makeViewBbs(viewPtrs, bboxes);

        double xSpacing = spacingX.getValue();    //in mm/scale
        double ySpacing = spacingY.getValue();    //in mm/scale

        double bigRow    = 0.0;
        double bigCol    = 0.0;
        for (auto& b: bboxes) {
            if (!b.IsValid()) {
                continue;
            }
            if (b.LengthX() > bigCol) {
                bigCol = b.LengthX();
            }
            if (b.LengthY() > bigRow) {
                bigRow = b.LengthY();
            }
        }

        //if we have iso's, make sure they fit the grid.
        if (viewPtrs[0] || viewPtrs[2]  || viewPtrs[7] ||  viewPtrs[9]) {
            bigCol = std::max(bigCol,bigRow);
            bigRow = bigCol;
        }

        if (viewPtrs[0] &&                          //iso
            bboxes[0].IsValid()) {
            position[0].x = -bigCol - xSpacing;
            position[0].y = bigRow + ySpacing;
        }
        if (viewPtrs[1] &&                         // T/B
            bboxes[1].IsValid()) {
            position[1].x = 0.0;
            position[1].y = bigRow + ySpacing;
        }
        if (viewPtrs[2] &&                         //iso
            bboxes[2].IsValid()) {
            position[2].x = bigCol + xSpacing;
            position[2].y = bigRow + ySpacing;
        }
        if (viewPtrs[3] &&                        // L/R
            bboxes[3].IsValid() &&
            bboxes[4].IsValid()) {
            position[3].x = -bigCol - xSpacing;
            position[3].y = 0.0;
        }
        if (viewPtrs[4] &&                       //Front
            bboxes[4].IsValid()) {
            position[4].x = 0.0;
            position[4].y = 0.0;
        }
        if (viewPtrs[5] &&                       // R/L
            bboxes[5].IsValid() &&
            bboxes[4].IsValid()) {
            position[5].x = bigCol + xSpacing;
            position[5].y = 0.0;
        }
        if (viewPtrs[6] &&
            bboxes[6].IsValid()) {    //"Rear"
            if (viewPtrs[5] &&
                bboxes[5].IsValid()) {
                position[6].x = position[5].x + bigCol + xSpacing;
                position[6].y = 0.0;
            }else if (viewPtrs[4] &&
                bboxes[4].IsValid()) {
                position[6].x = bigCol + xSpacing;
                position[6].y = 0.0;
            }
        }
        if (viewPtrs[7] &&
            bboxes[7].IsValid()) {             //iso
            position[7].x = -bigCol - xSpacing;
            position[7].y = -bigRow - ySpacing;
        }
        if (viewPtrs[8] &&                     // B/T
            bboxes[8].IsValid() &&
            bboxes[4].IsValid()) {
            position[8].x = 0.0;
            position[8].y = -bigRow - ySpacing;
        }
        if (viewPtrs[9] &&                    //iso
            bboxes[9].IsValid()) {
            position[9].x = bigCol + xSpacing;
            position[9].y = -bigRow - ySpacing;
        }
        result.x = position[viewIndex].x;
        result.y = position[viewIndex].y;
    } else {
        result.x = viewPtrs[viewIndex]->X.getValue();
        result.y = viewPtrs[viewIndex]->Y.getValue();
    }
    return result;
}

int DrawProjGroup::getViewIndex(const char *viewTypeCStr) const
{
    int result = 4;                                        //default to front view's position
    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    DrawPage* dp = findParentPage();
    if (ProjectionType.isValue("Default")) {
        if (dp != nullptr) {
            projType = dp->ProjectionType.getValueAsString();
        } else {
            Base::Console().Warning("DPG: %s - can not find parent page. Using default Projection Type. (1)\n",
                                    getNameInDocument());
            int projConv = getDefProjConv();
            projType = ProjectionTypeEnums[projConv + 1];
        }
    } else {
        projType = ProjectionType.getValueAsString();
    }

    if ( strcmp(projType, "Third Angle") == 0 ||
         strcmp(projType, "First Angle") == 0    ) {
        //   Third Angle:  FTL  T  FTRight          0  1  2
        //                  L   F   Right   Rear    3  4  5  6
        //                 FBL  B  FBRight          7  8  9
        //
        //   First Angle:  FBRight  B  FBL          0  1  2
        //                  Right   F   L  Rear     3  4  5  6
        //                 FTRight  T  FTL          7  8  9

        bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
        if (strcmp(viewTypeCStr, "Front") == 0) {
            result = 4;
        } else if (strcmp(viewTypeCStr, "Left") == 0) {
            result = thirdAngle ? 3 : 5;
        } else if (strcmp(viewTypeCStr, "Right") == 0) {
            result = thirdAngle ? 5 : 3;
        } else if (strcmp(viewTypeCStr, "Top") == 0) {
            result = thirdAngle ? 1 : 8;
        } else if (strcmp(viewTypeCStr, "Bottom") == 0) {
            result = thirdAngle ? 8 : 1;
        } else if (strcmp(viewTypeCStr, "Rear") == 0) {
            result = 6;
        } else if (strcmp(viewTypeCStr, "FrontTopLeft") == 0) {
            result = thirdAngle ? 0 : 9;
        } else if (strcmp(viewTypeCStr, "FrontTopRight") == 0) {
            result = thirdAngle ? 2 : 7;
        } else if (strcmp(viewTypeCStr, "FrontBottomLeft") == 0) {
            result = thirdAngle ? 7 : 2;
        } else if (strcmp(viewTypeCStr, "FrontBottomRight") == 0) {
            result = thirdAngle ? 9 : 0;
        } else {
            throw Base::TypeError("Unknown view type in DrawProjGroup::getViewIndex()");
        }
    } else {
        throw Base::ValueError("Unknown Projection convention in DrawProjGroup::getViewIndex()");
    }
    return result;
}

void DrawProjGroup::arrangeViewPointers(DrawProjGroupItem *viewPtrs[10]) const
{
    for (int i=0; i<10; ++i) {
        viewPtrs[i] = NULL;
    }

    // Determine layout - should be either "First Angle" or "Third Angle"
    const char* projType;
    if (ProjectionType.isValue("Default")) {
        DrawPage* dp = findParentPage();
        if (dp != nullptr) {
            projType = dp->ProjectionType.getValueAsString();
        } else {
            Base::Console().Error("DPG:arrangeViewPointers - %s - DPG is not on a page!\n",
                                    getNameInDocument());
            Base::Console().Warning("DPG:arrangeViewPointers - using system default Projection Type\n",
                                    getNameInDocument());
            int projConv = getDefProjConv();
            projType = ProjectionTypeEnums[projConv + 1];
        }
    } else {
        projType = ProjectionType.getValueAsString();
    }

    // Iterate through views and populate viewPtrs
    if ( strcmp(projType, "Third Angle") == 0 ||
         strcmp(projType, "First Angle") == 0    ) {
        //   Third Angle:  FTL  T  FTRight          0  1  2
        //                  L   F   Right   Rear    3  4  5  6
        //                 FBL  B  FBRight          7  8  9
        //
        //   First Angle:  FBRight  B  FBL          0  1  2
        //                  Right   F   L  Rear     3  4  5  6
        //                 FTRight  T  FTL          7  8  9

        bool thirdAngle = (strcmp(projType, "Third Angle") == 0);
        for (auto it : Views.getValues()) {
            auto oView( dynamic_cast<DrawProjGroupItem *>(it) );
            if (oView == nullptr) {
                //if an element in Views is not a DPGI, something really bad has happened somewhere
                Base::Console().Log("PROBLEM - DPG::arrangeViewPointers - non DPGI in Views! %s\n",
                                    getNameInDocument());
                throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
            } else {
                const char *viewTypeCStr = oView->Type.getValueAsString();
                if (strcmp(viewTypeCStr, "Front") == 0) {
                  //viewPtrs[thirdAngle ? 4 : 4] = oView;
                    viewPtrs[4] = oView;
                } else if (strcmp(viewTypeCStr, "Left") == 0) {
                    viewPtrs[thirdAngle ? 3 : 5] = oView;
                } else if (strcmp(viewTypeCStr, "Right") == 0) {
                    viewPtrs[thirdAngle ? 5 : 3] = oView;
                } else if (strcmp(viewTypeCStr, "Top") == 0) {
                    viewPtrs[thirdAngle ? 1 : 8] = oView;
                } else if (strcmp(viewTypeCStr, "Bottom") == 0) {
                    viewPtrs[thirdAngle ? 8 : 1] = oView;
                } else if (strcmp(viewTypeCStr, "Rear") == 0) {
                    viewPtrs[6] = oView;
                } else if (strcmp(viewTypeCStr, "FrontTopLeft") == 0) {
                    viewPtrs[thirdAngle ? 0 : 9] = oView;
                } else if (strcmp(viewTypeCStr, "FrontTopRight") == 0) {
                    viewPtrs[thirdAngle ? 2 : 7] = oView;
                } else if (strcmp(viewTypeCStr, "FrontBottomLeft") == 0) {
                    viewPtrs[thirdAngle ? 7 : 2] = oView;
                } else if (strcmp(viewTypeCStr, "FrontBottomRight") == 0) {
                    viewPtrs[thirdAngle ? 9 : 0] = oView;
                } else {
                    Base::Console().Warning("DPG: %s - unknown view type: %s. \n",
                                            getNameInDocument(),viewTypeCStr);
                    throw Base::TypeError("Unknown view type in DrawProjGroup::arrangeViewPointers.");
                }
            }
        }
    } else {
        Base::Console().Warning("DPG: %s - unknown Projection convention: %s\n",getNameInDocument(),projType);
        throw Base::ValueError("Unknown Projection convention in DrawProjGroup::arrangeViewPointers");
    }
}

void DrawProjGroup::makeViewBbs(DrawProjGroupItem *viewPtrs[10],
                                          Base::BoundBox3d bboxes[10],
                                          bool documentScale) const
{
    for (int i = 0; i < 10; ++i)
        if (viewPtrs[i]) {
            bboxes[i] = viewPtrs[i]->getBoundingBox();
            if (!documentScale) {
                double scale = 1.0 / viewPtrs[i]->getScale();    //convert bbx to 1:1 scale
                bboxes[i].ScaleX(scale);
                bboxes[i].ScaleY(scale);
                bboxes[i].ScaleZ(scale);
            }
        } else {
            // BoundBox3d defaults to length=(FLOAT_MAX + -FLOAT_MAX)
            bboxes[i].ScaleX(0);
            bboxes[i].ScaleY(0);
            bboxes[i].ScaleZ(0);
        }
}

void DrawProjGroup::recomputeChildren(void)
{
//    Base::Console().Message("DPG::recomputeChildren()\n");
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->recomputeFeature();
        }
    }
}

void DrawProjGroup::autoPositionChildren(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->autoPosition();
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed Scale
 */
void DrawProjGroup::updateChildrenScale(void)
{
//    Base::Console().Message("DPG::updateChildrenScale\n");
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenScale - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else  if(view->Scale.getValue()!=Scale.getValue()) {
            view->Scale.setValue(Scale.getValue());
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed Source
 */
void DrawProjGroup::updateChildrenSource(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenSource - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else if (view->Source.getValues() != Source.getValues()) {
            view->Source.setValues(Source.getValues());
        }
    }
}

/*!
 * tell children DPGIs that parent DPG has changed LockPosition
 * (really for benefit of QGIV on Gui side)
 */
void DrawProjGroup::updateChildrenLock(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenLock - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        }
    }
}

void DrawProjGroup::updateChildrenEnforce(void)
{
    for( const auto it : Views.getValues() ) {
        auto view( dynamic_cast<DrawProjGroupItem *>(it) );
        if (view == nullptr) {
            //if an element in Views is not a DPGI, something really bad has happened somewhere
            Base::Console().Log("PROBLEM - DPG::updateChildrenEnforce - non DPGI entry in Views! %s\n",
                                    getNameInDocument());
            throw Base::TypeError("Error: projection in DPG list is not a DPGI!");
        } else {
            view->enforceRecompute();
        }
    }
}

/*!
 * check if ProjectionGroup fits on Page
 */
bool DrawProjGroup::checkFit(TechDraw::DrawPage* p) const
{
    bool result = true;

    QRectF viewBox = getRect();
    double fudge = 1.1;
    double maxWidth = viewBox.width() * fudge;
    double maxHeight = viewBox.height() * fudge;

    if ( (maxWidth > p->getPageWidth()) ||
         (maxHeight > p->getPageHeight()) ) {
        result = false;
    }

    if (ScaleType.isValue("Automatic")) {                        //expand if too small
        double magnifyLimit = 0.60;
        if ( (maxWidth < p->getPageWidth() * magnifyLimit) &&
             (maxHeight < p->getPageHeight() * magnifyLimit) ) {
            result = false;
        }
    }
    return result;
}


App::Enumeration DrawProjGroup::usedProjectionType(void)
{
    //TODO: Would've been nice to have an Enumeration(const PropertyEnumeration &) constructor
    App::Enumeration ret(ProjectionTypeEnums, ProjectionType.getValueAsString());
    if (ret.isValue("Default")) {
        TechDraw::DrawPage * page = getPage();
        if ( page != NULL ) {
            ret.setValue(page->ProjectionType.getValueAsString());
        }
    }
    return ret;
}

bool DrawProjGroup::hasAnchor(void)
{
    bool result = false;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj != nullptr) {
        result = true;
    }
    return result;
}

TechDraw::DrawProjGroupItem* DrawProjGroup::getAnchor(void)
{
    DrawProjGroupItem* result = nullptr;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj != nullptr) {
        result = static_cast<DrawProjGroupItem*>(docObj);
    }
    return result;
}

void DrawProjGroup::setAnchorDirection(const Base::Vector3d dir)
{
    App::DocumentObject* docObj = Anchor.getValue();
    DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(docObj);
    item->Direction.setValue(dir);
}

Base::Vector3d DrawProjGroup::getAnchorDirection(void)
{
    Base::Vector3d result;
    App::DocumentObject* docObj = Anchor.getValue();
    if (docObj != nullptr) {
        DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(docObj);
        result = item->Direction.getValue();
    } else {
        Base::Console().Log("ERROR - DPG::getAnchorDir - no Anchor!!\n");
    }
    return result;
}

//*************************************
//* view direction manipulation routines
//*************************************

//note: must calculate all the new directions before applying any of them or
// the process will get confused.
void DrawProjGroup::updateSecondaryDirs()
{
    DrawProjGroupItem* anchor = getAnchor();
    Base::Vector3d anchDir = anchor->Direction.getValue();
    Base::Vector3d anchRot = anchor->getXDirection();

    std::map<std::string, std::pair<Base::Vector3d,Base::Vector3d> > saveVals;
    std::string key;
    std::pair<Base::Vector3d, Base::Vector3d> data;
    for (auto& docObj: Views.getValues()) {
        std::pair<Base::Vector3d,Base::Vector3d> newDirs;
        std::string pic;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        ProjItemType t = static_cast<ProjItemType>(v->Type.getValue());
        switch (t) {
            case Front :
                data.first = anchDir;
                data.second = anchRot;
                key = "Front";
                saveVals[key] = data;
                break;
            case Rear :
                key = "Rear";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Left :
                key = "Left";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Right :
            key = "Right";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Top :
                key = "Top";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case Bottom :
                key = "Bottom";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontTopLeft :
                key = "FrontTopLeft";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontTopRight :
                key = "FrontTopRight";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontBottomLeft :
                key = "FrontBottomLeft";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            case FrontBottomRight :
                key = "FrontBottomRight";
                newDirs = getDirsFromFront(key);
                saveVals[key] = newDirs;
                break;
            default: {
                //TARFU invalid secondary type
                Base::Console().Message("ERROR - DPG::updateSecondaryDirs - invalid projection type\n");
            }
        }
    }

    for (auto& docObj: Views.getValues()) {
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        std::string type = v->Type.getValueAsString();
        data = saveVals[type];
        v->Direction.setValue(data.first);
        v->Direction.purgeTouched();
        v->XDirection.setValue(data.second);
        v->XDirection.purgeTouched();
    }
    recomputeChildren();
}

void DrawProjGroup::rotateRight()
{
//Front -> Right -> Rear -> Left -> Front
    std::pair<Base::Vector3d,Base::Vector3d> newDirs;
    newDirs  = getDirsFromFront("Left");
    DrawProjGroupItem* anchor = getAnchor();
    anchor->Direction.setValue(newDirs.first);
    anchor->XDirection.setValue(newDirs.second);
    updateSecondaryDirs();
}

void DrawProjGroup::rotateLeft()
{
//Front -> Left -> Rear -> Right -> Front
    std::pair<Base::Vector3d,Base::Vector3d> newDirs;
    newDirs  = getDirsFromFront("Right");
    DrawProjGroupItem* anchor = getAnchor();
    anchor->Direction.setValue(newDirs.first);
    anchor->XDirection.setValue(newDirs.second);
    updateSecondaryDirs();
}

void DrawProjGroup::rotateUp()
{
//Front -> Top -> Rear -> Bottom -> Front
    std::pair<Base::Vector3d,Base::Vector3d> newDirs;
    newDirs  = getDirsFromFront("Bottom");
    DrawProjGroupItem* anchor = getAnchor();
    anchor->Direction.setValue(newDirs.first);
    anchor->XDirection.setValue(newDirs.second);
    updateSecondaryDirs();
}

void DrawProjGroup::rotateDown()
{
//Front -> Bottom -> Rear -> Top -> Front
    std::pair<Base::Vector3d,Base::Vector3d> newDirs;
    newDirs  = getDirsFromFront("Top");
    DrawProjGroupItem* anchor = getAnchor();
    anchor->Direction.setValue(newDirs.first);
    anchor->XDirection.setValue(newDirs.second);
    updateSecondaryDirs();
}

void DrawProjGroup::spinCW()
{
//Top -> Right -> Bottom -> Left -> Top
    DrawProjGroupItem* anchor = getAnchor();
    double angle = M_PI / 2.0;
    Base::Vector3d org(0.0,0.0,0.0);
    Base::Vector3d curRot = anchor->getXDirection();
    Base::Vector3d curDir = anchor->Direction.getValue();
    Base::Vector3d newRot = DrawUtil::vecRotate(curRot,angle,curDir,org);
    anchor->XDirection.setValue(newRot);
    updateSecondaryDirs();
}

void DrawProjGroup::spinCCW()
{
//Top -> Left -> Bottom -> Right -> Top
    DrawProjGroupItem* anchor = getAnchor();
    double angle = M_PI / 2.0;
    Base::Vector3d org(0.0,0.0,0.0);
    Base::Vector3d curRot = anchor->getXDirection();
    Base::Vector3d curDir = anchor->Direction.getValue();
    Base::Vector3d newRot = DrawUtil::vecRotate(curRot,-angle,curDir,org);
    anchor->XDirection.setValue(newRot);

    updateSecondaryDirs();
}

std::vector<DrawProjGroupItem*> DrawProjGroup::getViewsAsDPGI()
{
    std::vector<DrawProjGroupItem*> result;
    auto views = Views.getValues();
    for (auto& v:views) {
        DrawProjGroupItem* item = static_cast<DrawProjGroupItem*>(v);
        result.push_back(item);
    }
    return result;
}

int DrawProjGroup::getDefProjConv(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                               GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    int defProjConv = hGrp->GetInt("ProjectionAngle",0);
    return defProjConv;
}

/*!
 *dumps the current iso DPGI's
 */
void DrawProjGroup::dumpISO(const char * title)
{
    Base::Console().Message("DPG ISO: %s\n", title);
    for (auto& docObj: Views.getValues()) {
        Base::Vector3d dir;
        Base::Vector3d axis;
        DrawProjGroupItem* v = static_cast<DrawProjGroupItem*>(docObj);
        std::string t = v->Type.getValueAsString();
        dir = v->Direction.getValue();
        axis = v->getXDirection();

        Base::Console().Message("%s:  %s/%s\n",
                                t.c_str(),DrawUtil::formatVector(dir).c_str(),DrawUtil::formatVector(axis).c_str());
    }
}

PyObject *DrawProjGroup::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawProjGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
