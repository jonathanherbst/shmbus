#pragma once

#include <stdexcept>

namespace shmbus {

class error : public std::runtime_error
{
protected:
    error(const std::string& what) :
    std::runtime_error(what)
    {}
};

class out_of_mandatory_consumers : public error
{
public:
    out_of_mandatory_consumers() :
    error("there is no more space for mandatory consumers")
    {}
};

class zero_id : public error
{
public:
    zero_id() :
    error("mandatory consumer id must not be all zero")
    {}
};

}