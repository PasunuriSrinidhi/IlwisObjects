/*IlwisObjects is a framework for analysis, processing and visualization of remote sensing and gis data
Copyright (C) 2018  52n North

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "kernel.h"
#include "raster.h"
#include "symboltable.h"
#include "ilwisoperation.h"
#include "addrasterband.h"

using namespace Ilwis;
using namespace BaseOperations;

REGISTER_OPERATION(AddRasterBand)

AddRasterBand::AddRasterBand()
{
}

AddRasterBand::AddRasterBand(quint64 metaid, const Ilwis::OperationExpression &expr) :
    OperationImplementation(metaid, expr)
{
}

bool AddRasterBand::execute(ExecutionContext *ctx, SymbolTable& symTable)
{
    if (_prepState == sNOTPREPARED)
        if((_prepState = prepare(ctx, symTable)) != sPREPARED)
            return false;

    PixelIterator bandIter(_band);
    if ( hasType(_inputRaster->stackDefinition().domain()->ilwisType(), itITEMDOMAIN|itTEXTDOMAIN))
         _inputRaster->band(sUNDEF, bandIter);
     else
        _inputRaster->band(rUNDEF, bandIter);

    ctx->_additionalInfo[INPUTISOUTPUTFLAG] = _inputRaster->id();
	logOperation(_inputRaster, _expression, { _band });
    setOutput(_inputRaster, ctx, symTable);

    return true;
}

Ilwis::OperationImplementation *AddRasterBand::create(quint64 metaid, const Ilwis::OperationExpression &expr)
{
        return new AddRasterBand(metaid, expr);
}

Ilwis::OperationImplementation::State AddRasterBand::prepare(ExecutionContext *ctx, const SymbolTable &st)
{
    OperationImplementation::prepare(ctx,st);
    QString rasterUrl = _expression.input<QString>(0);
    if (!_inputRaster.prepare(rasterUrl)){
        kernel()->issues()->log(TR("Raster can not be loaded:") + rasterUrl);
        return sPREPAREFAILED;
    }
    rasterUrl = _expression.input<QString>(1);
    if (!_band.prepare(rasterUrl)){
        kernel()->issues()->log(TR("Raster can not be loaded:") + rasterUrl);
        return sPREPAREFAILED;
    }
    if ( _inputRaster->size().zsize() == 0){
        _inputRaster->datadefRef() = _band->datadef();
    }else {
        if(!_inputRaster->datadef().domain()->isCompatibleWith(_band->datadef().domain().ptr())){
            kernel()->issues()->log(TR("Domains of the input raster and the band are not compatible:"));
            return sPREPAREFAILED;
        }
    }
    return sPREPARED;
}

quint64 AddRasterBand::createMetadata()
{

    OperationResource operation({"ilwis://operations/addrasterband"});
    operation.setSyntax("addrasterband(inputraster, band)");
    operation.setDescription(TR("adds a new band at the top of the stack and returns the input raster with the extra band"));
    operation.setInParameterCount({2});
    operation.addInParameter(0,itRASTER , TR("input rastercoverage"),TR("input rastercoverage with any domain"));
    operation.addInParameter(1,itRASTER, TR("new band (raster)"), TR("band to be added with a domain compatible with the input raster"));
    operation.setOutParameterCount({1});
    operation.addOutParameter(0,itRASTER , TR("output raster"),TR("raster with extra band"));
    operation.setKeywords("raster,workflow");

    operation.checkAlternateDefinition();
    operation.checkAlternateDefinition();
    mastercatalog()->addItems({operation});
    return operation.id();

}

