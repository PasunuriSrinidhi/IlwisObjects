#include "kernel.h"
#include "ilwisdata.h"
#include "coverage.h"
#include "symboltable.h"
#include "operationExpression.h"
#include "operationmetadata.h"
#include "commandhandler.h"
#include "operation.h"
#include "mastercatalog.h"
#include "uicontextmodel.h"
#include "coveragedisplay/draweroperation.h"
#include "coveragedisplay/layermodel.h"
#include "coveragedisplay/layermanager.h"
#include "coveragedisplay/rootlayermodel.h"
#include "setviewextents.h"

using namespace Ilwis;
using namespace Ui;

REGISTER_OPERATION(SetViewExtent)

SetViewExtent::SetViewExtent()
{
}

SetViewExtent::SetViewExtent(quint64 metaid, const Ilwis::OperationExpression &expr) : DrawerOperation(metaid, expr)
{

}

void SetViewExtent::RecenterZoomHorz(Envelope & cbZoom, const Envelope & cbMap)
{
	double zwidth = cbZoom.xlength() - 1;
	if (zwidth > cbMap.xlength() - 1) {
		double delta = (zwidth - (cbMap.xlength() - 1)) / 2.0;
		cbZoom.min_corner().x = cbMap.min_corner().x - delta;
		cbZoom.max_corner().x = cbMap.max_corner().x + delta;
	} else {
		if ( cbZoom.max_corner().x > cbMap.max_corner().x) {
			cbZoom.max_corner().x = cbMap.max_corner().x;
			cbZoom.min_corner().x = cbZoom.max_corner().x - zwidth;
		}
		if ( cbZoom.min_corner().x < cbMap.min_corner().x) {
			cbZoom.min_corner().x = cbMap.min_corner().x;
			cbZoom.max_corner().x = cbZoom.min_corner().x + zwidth;
		}
	}
}

void SetViewExtent::RecenterZoomVert(Envelope & cbZoom, const Envelope & cbMap)
{
    double zheight = cbZoom.ylength() - 1;
	if (zheight > cbMap.ylength() - 1) {
		double delta = (zheight - (cbMap.ylength() - 1)) / 2.0;
		cbZoom.min_corner().y = cbMap.min_corner().y - delta;
		cbZoom.max_corner().y = cbMap.max_corner().y + delta;
	} else {
		if ( cbZoom.max_corner().y > cbMap.max_corner().y) {
			cbZoom.max_corner().y = cbMap.max_corner().y;
			cbZoom.min_corner().y = cbZoom.max_corner().y - zheight;
		}
		if ( cbZoom.min_corner().y < cbMap.min_corner().y) {
			cbZoom.min_corner().y = cbMap.min_corner().y;
			cbZoom.max_corner().y = cbZoom.min_corner().y + zheight;
		}
	}
}

bool SetViewExtent::execute(ExecutionContext *ctx, SymbolTable &symTable)
{
    if (_prepState == sNOTPREPARED)
        if((_prepState = prepare(ctx,symTable)) != sPREPARED)
            return false;

    if ( _entiremap){
		layerManager()->wholeMap();
    }else{
        double area = _newExtents.area();
        if ( area > 0){
            Envelope cbMap = layerManager()->rootLayer()->coverageEnvelope();
            if (_newExtents.xlength() > cbMap.xlength() && _newExtents.ylength() > cbMap.ylength())
                layerManager()->wholeMap();
            else {
                RecenterZoomHorz(_newExtents, cbMap);
                RecenterZoomVert(_newExtents, cbMap);
                layerManager()->rootLayer()->zoomEnvelope(_newExtents);
            }
        }
    }
	layerManager()->needUpdate(true);

    return true;
}

Ilwis::OperationImplementation *SetViewExtent::create(quint64 metaid, const Ilwis::OperationExpression &expr)
{
    return new SetViewExtent(metaid, expr);
}

Ilwis::OperationImplementation::State SetViewExtent::prepare(ExecutionContext *ctx, const SymbolTable &)
{
    if (!getViewId(_expression.input<QString>(0))){
        return sPREPAREFAILED;
    }

    auto checkCoords =[](const OperationExpression& expr, int index)->double{
        bool ok;
        double value = expr.parm(index).value().toDouble(&ok);
        if (!ok){
            ERROR3(ERR_ILLEGAL_PARM_3,"coordinate", expr.parm(index).value(), expr.toString());
            return rUNDEF;
        }
        return value;
    };
    double xmin=rUNDEF, ymin=rUNDEF, xmax=rUNDEF, ymax=rUNDEF;
    if ( _expression.parameterCount() == 5){
        xmin = checkCoords(_expression,1);
        ymin = checkCoords(_expression,2);
        xmax = checkCoords(_expression,3);
        ymax = checkCoords(_expression,4);
    }else {
        QStringList parts = _expression.input<QString>(1).split(" ");
        if ( parts.size() == 1){
            if ( parts[0].toLower() == "entiremap"){
                _entiremap = true;
            }
        }
        else if ( !(parts.size() == 4 || parts.size() == 6)){
            ERROR3(ERR_ILLEGAL_PARM_3,"coordinate list", _expression.parm(1).value(), _expression.toString());
            return sPREPAREFAILED;
        }
        if ( parts.size() == 4){
            xmin = parts[0].toDouble();
            ymin = parts[1].toDouble();
            xmax = parts[2].toDouble();
            ymax = parts[3].toDouble();
        } else if(parts.size() == 6){
            xmin = parts[0].toDouble();
            ymin = parts[1].toDouble();
            xmax = parts[3].toDouble();
            ymax = parts[4].toDouble();
        }
    }

    _newExtents = Envelope(Coordinate(xmin, ymin), Coordinate(xmax, ymax));
    if ( !_newExtents.isValid() && _entiremap != true)
        return sPREPAREFAILED;

    return sPREPARED;
}

quint64 SetViewExtent::createMetadata()
{
    OperationResource operation({"ilwis://operations/setviewextent"});
    operation.setSyntax("setviewextent(viewid, xmin, ymin, xmax, ymax)");
    operation.setDescription(TR("changes the view extent"));
    operation.setInParameterCount({2,5});
    operation.addInParameter(0,itINTEGER , TR("view id"),TR("id of the view to which this drawer has to be added"));
    operation.addInParameter(1,itDOUBLE|itSTRING , TR("minimum x coordinate or (in the two parameter version) ogc compatible coordinate string or size-directive"));
    operation.addInParameter(2,itDOUBLE , TR("minimum y coordinate"));
    operation.addInParameter(3,itDOUBLE , TR("maximum x coordinate"));
    operation.addInParameter(4,itDOUBLE , TR("maximum y coordinate"));
    operation.setOutParameterCount({0});
    operation.setKeywords("visualization");

    mastercatalog()->addItems({operation});
    return operation.id();
}

