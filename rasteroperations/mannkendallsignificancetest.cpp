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

#include <iostream>
#include "kernel.h"
#include "ilwisdata.h"
#include "symboltable.h"
#include "domain.h"
#include "raster.h"
#include "pixeliterator.h"
#include "operationExpression.h"
#include "operationmetadata.h"
#include "operationhelper.h"
#include "operationhelpergrid.h"
#include "commandhandler.h"
#include "operation.h"
#include "mannkendallsignificancetest.h"
#include "boost/math/distributions/normal.hpp"

using namespace Ilwis;
using namespace RasterOperations;
using boost::math::normal;

REGISTER_OPERATION(MannKendallSignificanceTest)

MannKendallSignificanceTest::MannKendallSignificanceTest()
{
}

MannKendallSignificanceTest::MannKendallSignificanceTest(quint64 metaid, const Ilwis::OperationExpression &expr) :
	OperationImplementation(metaid, expr)
{
}

double MannKendallSignificanceTest::trendValue(const std::vector<double>& stackColumn, std::vector<int>& ties) const{
	std::vector<int> sumResult(stackColumn.size(), 0);

	for (int i = 0; i < stackColumn.size(); ++i) {
		std::vector<int> deltaDirection(stackColumn.size(),0);
		double init = stackColumn[i];
		int repeats = 0;
		for (int j = 0; j < stackColumn.size(); ++i) {
			if (j > i) {
				double v = stackColumn[j];
				double d = init - v;
				if (isNumericalUndef(v)) {
					deltaDirection[j] = 0;
					repeats++;
				}else
					deltaDirection[j] = d > 0 ? 1 : -1;
			}
		}
		if ( repeats > 0)
			ties.push_back(repeats);
		double sum = std::accumulate(deltaDirection.begin(), deltaDirection.end(), 0);
		sumResult[i] = sum;
	}

	double totalSum = std::accumulate(sumResult.begin(), sumResult.end(), 0);
		
	return totalSum;
}

double MannKendallSignificanceTest::calcVarS(int n, const std::vector<int>&ties) const{
	auto lambda = [](int init, int v)->int { return v * (v - 1)*(2 * v + 5); };
	int tieFactor = std::accumulate(ties.begin(),  ties.end(), 0, lambda);
	int sizeFactor = lambda(0,n);

	return (sizeFactor - tieFactor) / 18;

}

double MannKendallSignificanceTest::calcZ(int s, double varS) const{
	if (s > 0) {
		return (s - 1) / std::sqrt(varS);
	}
	if (s == 0)
		return 0;
	return (s + 1) / std::sqrt(varS);
}

bool MannKendallSignificanceTest::execute(ExecutionContext *ctx, SymbolTable &symTable)
{
	if (_prepState == sNOTPREPARED)
		if ((_prepState = prepare(ctx, symTable)) != sPREPARED)
			return false;

	double inValue;
	bool xchanged = false;
	std::vector<double> zcolumn;
	zcolumn.reserve(_inputRaster->size().zsize());
	PixelIterator iterIn(_inputRaster, BoundingBox(), PixelIterator::fZXY);
	int count = 0;
	for (auto& value : _outputRaster) {
		while (!xchanged) {
			zcolumn.push_back(*iterIn);
			++iterIn;
			xchanged = iterIn.xchanged();
		}
		updateTranquilizer(++count, 100);
		std::vector<int> ties;
		double s = trendValue(zcolumn, ties);
		zcolumn.clear(); // next column
		xchanged = false; // reset flag as we are in the next column now
		double varS = calcVarS(_inputRaster->size().zsize(), ties);
		double z = calcZ(s, varS);
		normal ndis;
		double prob = cdf(ndis, z);
		value = prob > 0;

		updateTranquilizer(iterIn.linearPosition(), 1000);

	}

	QVariant value;
	value.setValue<IRasterCoverage>(_outputRaster);
	logOperation(_outputRaster, _expression);
	ctx->setOutput(symTable, value, _outputRaster->name(), itRASTER, _outputRaster->resource());
	return true;

}

Ilwis::OperationImplementation *MannKendallSignificanceTest::create(quint64 metaid, const Ilwis::OperationExpression &expr)
{
	return new MannKendallSignificanceTest(metaid, expr);
}

Ilwis::OperationImplementation::State MannKendallSignificanceTest::prepare(ExecutionContext *, const Ilwis::SymbolTable &symTable)
{
	OperationHelper::check([&]()->bool { return _inputRaster.prepare(_expression.input<QString>(0), itRASTER); },
		{ ERR_COULD_NOT_LOAD_2,_expression.input<QString>(0), "" });

	if (_inputRaster->size().zsize() < 2 ) {
		kernel()->issues()->log(TR("Multi band raster must have more than 10 bands"));
		return sPREPAREFAILED;
	}

	OperationHelper::check([&]()->bool { return _significanceDomain.prepare(_expression.input<QString>(1), itDOMAIN); },
		{ ERR_COULD_NOT_LOAD_2,_expression.input<QString>(1), "" });

	if (_significanceDomain->ilwisType() != itITEMDOMAIN) {
		kernel()->issues()->log(TR("Significance domain must be an item domain"));
		return sPREPAREFAILED;
	}

	OperationHelper::check([&]()->bool { bool ok;  _significanceValue = _expression.input<double>(2,ok); return ok; },
		{ ERR_NO_INITIALIZED_1,"significance value", _expression.input<QString>(2) });

	OperationHelperRaster::initialize(_inputRaster, _outputRaster, itCOORDSYSTEM | itGEOREF| itENVELOPE| itRASTERSIZE);
	if (!_outputRaster.isValid()) {
		ERROR1(ERR_NO_INITIALIZED_1, "output rastercoverage");
		return sPREPAREFAILED;
	}
	std::vector<double> indexes = { 0 };
	_outputRaster->setDataDefintions(_significanceDomain, indexes);

	initialize(_outputRaster->size().linearSize());

	return sPREPARED;

}

quint64 MannKendallSignificanceTest::createMetadata()
{
	OperationResource operation({ "ilwis://operations/mannkendallsignificancetest" });
	operation.setLongName("Mann-Kendall Significance Test");
	operation.setSyntax("MannKendallSignificanceTest(raster,domain,number)");
	operation.setInParameterCount({ 3 });
	operation.addInParameter(0, itRASTER, TR("Multi-band raster"), TR("A multi band raster with a numeric domain"));
	operation.addInParameter(1, itDOMAIN, TR("Output domain"), TR("An item domain that indicates the semantcis of signinfance for the output map"));
	operation.addInParameter(2, itDOUBLE, TR("Significance level"), TR("The probability that a pixel(value) belongs to a certain hypothesis"));
	operation.setOutParameterCount({ 1 });
	operation.addInParameter(6, itRASTER , TR("significance raster"), TR("A raster with pixel class values indicating the significance of a certain process"));
	operation.setKeywords("raster, statistics, trends");

	mastercatalog()->addItems({ operation });
	return operation.id();
}
