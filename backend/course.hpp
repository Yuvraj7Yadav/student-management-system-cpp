#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct Course {
    int id{0};
    std::string course_name;
    std::string department;
    std::string duration;
    std::string description;
    int student_count{0};

    nlohmann::json to_json() const;
    static Course from_json(const nlohmann::json& j);
    bool validate(std::string& error_msg) const;
};
