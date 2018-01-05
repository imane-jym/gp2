#ifndef QUERYRESULT_H
#define QUERYRESULT_H

#include "Common.h"
#include "Field.h"

class QueryResult
{
    public:
        QueryResult(uint64 rowCount, uint32 fieldCount)
            : mFieldCount(fieldCount), mRowCount(rowCount), mCurrentRow(NULL) {}
        virtual ~QueryResult() {}

        virtual bool NextRow() = 0;

        Field* Fetch() const { return mCurrentRow; }

        const Field& operator [](int index) const { return mCurrentRow[index]; }

        uint32 GetFieldCount() const { return mFieldCount; }
        uint64 GetRowCount() const { return mRowCount; }

    protected:
        Field* mCurrentRow;
        uint32 mFieldCount;
        uint64 mRowCount;
};

#endif
