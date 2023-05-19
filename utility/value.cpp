#include "value.hpp"


#include <stdlib.h>
#include <sstream>
using std::stringstream;

Value::Value() : m_type(V_NULL)
{
}

Value::Value(bool value) : m_type(V_BOOL)
{
    *this = value;}

Value::Value(int value) : m_type(V_INT)
{
    *this = value;
}

Value::Value(double value) : m_type(V_DOUBLE)
{
    *this = value;
}

Value::Value(const char * value) : m_type(V_STRING), m_value(value)
{
}

Value::Value(const std::string & value) : m_type(V_STRING), m_value(value)
{
}

Value::~Value()
{
}

Value::Type Value::type() const
{
    return m_type;
}

void Value::type(Type type)
{
    m_type = type;
}

bool Value::is_null() const
{
    return m_type == V_NULL;
}

bool Value::is_int() const
{
    return m_type == V_INT;
}

bool Value::is_double() const
{
    return m_type == V_DOUBLE;
}

bool Value::is_string() const
{
    return m_type == V_STRING;
}

Value & Value::operator = (bool value)
{
    m_type = V_BOOL;
    m_value = value ? "true" : "false";
    return *this;
}

Value & Value::operator = (int value)
{
    m_type = V_INT;
    stringstream ss;
    ss << value;
    m_value = ss.str();
    return *this;
}

Value & Value::operator = (double value)
{
    m_type = V_DOUBLE;
    stringstream ss;
    ss << value;
    m_value = ss.str();
    return *this;
}

Value & Value::operator = (const char * value)
{
    m_type = V_STRING;
    m_value = value;
    return *this;
}

Value & Value::operator = (const std::string & value)
{
    m_type = V_STRING;
    m_value = value;
    return *this;
}

Value & Value::operator = (const Value & value)
{
    m_type = value.m_type;
    m_value = value.m_value;
    return *this;
}

bool Value::operator == (const Value & other)
{
    if (m_type != other.m_type)
    {
        return false;
    }
    return m_value == other.m_value;
}

bool Value::operator != (const Value & other)
{
    return !(m_value == other.m_value);
}


Value::operator bool()
{
    if (m_value == "true")
        return true;
    else if (m_value == "false")
        return false;
    return false;
}

Value::operator int() {

    return atoi(m_value.c_str());
}

Value::operator double() {

    return atof(m_value.c_str());
}

Value::operator std::string() {
    return m_value;
}

Value::operator std::string() const {

    return m_value;
}