#include "matcher.h"


bool matches(const Publication& pub, const Subscription& sub)
{
    for (const auto& f : sub.fields)
    {
        switch (f.fieldType)
        {
            case FieldType::COMPANY:
            {
                if (f.op == OperatorType::EQ)
                {
                    if (pub.company != f.stringValue)
                        return false;
                }
                else if (f.op == OperatorType::NEQ)
                {
                    if (pub.company == f.stringValue)
                        return false;
                }
                break;
            }

            case FieldType::DATE:
            {
                if (f.op == OperatorType::EQ)
                {
                    if (pub.date != f.stringValue)
                        return false;
                }
                else if (f.op == OperatorType::NEQ)
                {
                    if (pub.date == f.stringValue)
                        return false;
                }
                break;
            }

            case FieldType::VALUE:
            {
                if (f.op == OperatorType::GT)
                {
                    if (pub.value <= f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::GE)
                {
                    if (pub.value < f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::LT)
                {
                    if (pub.value >= f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::LE)
                {
                    if (pub.value > f.numericValue)
                        return false;
                }
                break;
            }

            case FieldType::DROP:
            {
                if (f.op == OperatorType::GT)
                {
                    if (pub.drop <= f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::GE)
                {
                    if (pub.drop < f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::LT)
                {
                    if (pub.drop >= f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::LE)
                {
                    if (pub.drop > f.numericValue)
                        return false;
                }
                break;
            }

            case FieldType::VARIATION:
            {
                if (f.op == OperatorType::GT)
                {
                    if (pub.variation <= f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::GE)
                {
                    if (pub.variation < f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::LT)
                {
                    if (pub.variation >= f.numericValue)
                        return false;
                }
                else if (f.op == OperatorType::LE)
                {
                    if (pub.variation > f.numericValue)
                        return false;
                }
                break;
            }
        }
    }

    return true;
}