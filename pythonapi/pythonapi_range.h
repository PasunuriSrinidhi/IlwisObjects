#ifndef PYTHONAPI_RANGE_H
#define PYTHONAPI_RANGE_H

#include "pythonapi_object.h"
#include<string>
#include<map>
#include "pythonapi_rangeiterator.h"
#include <QSharedPointer>
#include "pythonapi_util.h"
#include "pythonapi_domainitem.h"

typedef std::shared_ptr<Ilwis::Range> SPPRange;

namespace Ilwis {
class Range;
class NumericRange;
class DomainItem;
typedef QSharedPointer<DomainItem> SPDomainItem;
 class IntervalRange;
 class ColorPalette;
}

typedef struct _object PyObject;

namespace pythonapi {
    class NumericRange;
    class TimeInterval;
    class ColorPalette;
    class ContinuousColorRange;
    class NamedItemRange;
    class ThematicRange;
    class IndexedItemRange;
    class NumericItemRange;
    class Interval;

class Range: public Object{
    friend class Domain;
    friend class DataDefinition;
    friend class ItemDomain;
    template<typename OutputType, typename RangeType, typename IlwOutput,  typename IlwRange>
    friend class RangeIterator;
    friend class Interval;



public:
    bool __bool__() const;
    std::string __str__();
    IlwisTypes ilwisType();

    IlwisTypes valueType() const;
    PyObject* ensure(const PyObject* v, bool inclusive = true) const;
    bool contains(const PyObject *value, bool inclusive = true) const;

    bool isContinuous() const;
    PyObject* impliedValue(const PyObject *value) const;
    NumericRange *toNumericRange();
    pythonapi::NumericItemRange *toNumericItemRange();
    TimeInterval *toTimeInterval();
    ColorPalette *toColorPalette();
    ContinuousColorRange *toContinuousColorRange();
    ThematicRange *toThematicRange();
    NamedItemRange *toNamedItemRange();
    IndexedItemRange *toIndexedItemRange();

protected:
    SPPRange _range;
    Range(Ilwis::Range *rng);

    Range();
    virtual ~Range();
private:
};

class NumericRange : public Range {

public:
    NumericRange(double mi, double ma, double resolution = 0);
    NumericRange(const NumericRange &vr);
    NumericRange(Ilwis::NumericRange* nr);
    NumericRange();
    ~NumericRange();

    bool contains(double v, bool inclusive = true) const;
    double max() const;
    void setMax(double v);
    double min() const;
    void setMin(double v);
    double distance() const;

    void setResolution(double resolution);
    double resolution() const ;

    void set(const NumericRange& vr);

    pythonapi::NumericRangeIterator __iter__();
    pythonapi::NumericRangeIterator begin();
    pythonapi::NumericRangeIterator end();

    void clear();
};

class ItemRange : public Range {
public:
    virtual void add(PyObject *dItem) = 0;
    quint32 count();
    void remove(const std::string& name);
    void clear();
protected:
    ItemRange(Ilwis::ItemRange* rng)    ;
    ItemRange();
};

class NumericItemRange : public ItemRange{
public:
    NumericItemRange(Ilwis::IntervalRange* rng);
    NumericItemRange();
    void add(std::string name, double min, double max, double resolution=0);
    void add(PyObject *item);
    PyObject* listAll();
    pythonapi::Interval *item(quint32 index);
    pythonapi::Interval* item(std::string name);
    qint32 gotoIndex(qint32 index, qint32 step) const;
    NumericItemRange* clone();
};

class IndexedItemRange : public ItemRange{
public:
    IndexedItemRange();
    void add(PyObject* item);
    qint32 gotoIndex(qint32 index, qint32 step) const;
    IndexedItemRange* clone();
};

class NamedItemRange : public ItemRange {
public:
    NamedItemRange();
    NamedItemRange(Ilwis::ItemRange *rng);
    void add(PyObject *item);
    PyObject* listAll();
    qint32 gotoIndex(qint32 index, qint32 step) const;
    NamedItemRange* clone();
    pythonapi::NamedIdentifier* item(std::string name);
    pythonapi::NamedIdentifier* item(quint32 index);
};

class ThematicRange : public ItemRange {
public:
    ThematicRange();
    ThematicRange(Ilwis::ItemRange *rng);
    void add(std::string name, std::string id="", std::string descr="");
    void add(PyObject *item);
    PyObject* listAll();
    ThematicRange* clone();
    pythonapi::ThematicItem* item(quint32 index);
     pythonapi::ThematicItem* item(std::string name);
};


class ColorRangeBase{
    friend class ColorDomain;
    friend class ItemDomain;

public:
    ColorRangeBase();
    ColorRangeBase(IlwisTypes tp, ColorModel clrmodel);
    ColorModel defaultColorModel() const;
    void defaultColorModel(ColorModel m);

    static Color toColor(quint64 clrint, ColorModel clrModel, const std::string& name = "") ;
    static Color toColor(PyObject* v, ColorModel colortype, const std::string& name = "");

protected:
    Color qColorToColor( QColor qCol, const std::string& name = "") const;
    QColor colorToQColor(const Color& pyCol) const;
    SPPRange _colorRange;

private:
    std::string toString(const Color &clr, ColorModel clrType);
    ColorModel stringToColorModel(std::string clrmd);
    IlwisTypes _valuetype;
    ColorModel _defaultModel = ColorModel::cmRGBA;
};

class ContinuousColorRange : public ColorRangeBase, public Range{
public:
    ContinuousColorRange();
    ContinuousColorRange(Ilwis::Range *rng);
    ContinuousColorRange(const Color& clr1, const Color& clr2);
    ContinuousColorRange *clone() const;
    PyObject* ensure(const PyObject *v, bool inclusive = true) const;
    bool containsVar(const PyObject *v, bool inclusive = true) const;
    bool containsColor(const Color &clr, bool inclusive = true) const;
    bool containsRange(ColorRangeBase *v, bool inclusive = true) const;
    Color impliedValue(const PyObject* v) const;
};

class ColorPalette : public ItemRange, public ColorRangeBase{
public:
    ColorPalette();
    ColorPalette(Ilwis::ColorPalette *rng);

    Color item(quint32 raw) const;
    Color item(const std::string& name) const;
    Color itemByOrder(quint32 index) const;

    Color color(int index);

    void add(const Color& pyColor);
    void remove(const std::string& name);
    void clear();

    bool containsColor(const Color &clr, bool inclusive = true) const;
    bool containsRange(ColorRangeBase *v, bool inclusive = true) const;

    quint32 count();

    Color valueAt(quint32 index, ItemRange* rng);
    qint32 gotoIndex(qint32 index, qint32 step) const;

private:
    void add(PyObject* item);
    Color itemToColor(Ilwis::SPDomainItem item) const;
   // std::shared_ptr<Ilwis::Range> _rangeColor = pythonapi::ColorRange::_range;
   // std::shared_ptr<Ilwis::Range> _rangeItem = pythonapi::ItemRange::_range;
};

class TimeInterval : public NumericRange{
public:
    TimeInterval(IlwisTypes tp = itUNKNOWN);
    TimeInterval(const PyObject *beg, const PyObject *end, std::string step="", IlwisTypes tp = itUNKNOWN);

    //TimeInterval& operator=(const TimeInterval& tiv);
    PyObject* begin() const;
    PyObject* end() const ;
    void begin(const PyObject* t) ;
    void end(const PyObject* t);
    //Duration getStep() const { return _step;}
    bool contains(const std::string& value, bool inclusive = true) const;
    bool contains(const PyObject* value, bool inclusive = true) const;

    TimeInterval* clone() const ;

protected:
    TimeInterval(Ilwis::Range* ti);
};

}

#endif // PYTHONAPI_RANGE_H
