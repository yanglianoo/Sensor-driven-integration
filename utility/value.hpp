/**
 * @author:timer
 * @brief: 自定义的一个value类 支持 int double string bool
 * @date: 2023.4.20
*/
#pragma once

#include <iostream>
#include <string>
namespace SMW
{


    class Value
    {
    public:
        enum Type
        {
            V_NULL = 0,
            V_BOOL,
            V_INT,
            V_DOUBLE,
            V_STRING
        };

        Value();
        Value(bool value);
        Value(int value);
        Value(double value);
        Value(const char * value);
        Value(const std::string & value);
        ~Value();

        Type type() const;
        void type(Type type);
        bool is_null() const;
        bool is_int() const;
        bool is_double() const;
        bool is_string() const;

        Value & operator = (bool value);
        Value & operator = (int value);
        Value & operator = (double value);
        Value & operator = (const char * value);
        Value & operator = (const std::string & value);
        Value & operator = (const Value & value);

        bool operator == (const Value & other);
        bool operator != (const Value & other);
        
        // 可隐式地转换为 int类型 和 double 类型
        operator int();
        operator double();
        operator bool();
        operator std::string();
        operator std::string() const;

    private:
        Type m_type;
        std::string m_value;
    };
}


