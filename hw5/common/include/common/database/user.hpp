#pragma once

#include <string>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace common::database {

struct User {
    std::string id;
    std::string username;
    std::string first_name;
    std::string last_name;
    std::string email;
    std::string password_hash;
    std::string role;
    std::string created_at;
};

inline userver::formats::json::Value Serialize(
    const User& user,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = user.id;
    builder["username"] = user.username;
    builder["first_name"] = user.first_name;
    builder["last_name"] = user.last_name;
    builder["email"] = user.email;
    builder["role"] = user.role;
    builder["created_at"] = user.created_at;
    return builder.ExtractValue();
}

}
