#include "course.hpp"
#include <algorithm>

nlohmann::json Course::to_json() const {
    return nlohmann::json{
        {"id", id},
        {"course_name", course_name},
        {"department", department},
        {"duration", duration},
        {"description", description},
        {"student_count", student_count}
    };
}

Course Course::from_json(const nlohmann::json& j) {
    Course c;
    if (j.contains("id") && j["id"].is_number_integer()) {
        c.id = j["id"].get<int>();
    }
    if (j.contains("course_name") && j["course_name"].is_string()) {
        c.course_name = j["course_name"].get<std::string>();
    }
    if (j.contains("department") && j["department"].is_string()) {
        c.department = j["department"].get<std::string>();
    }
    if (j.contains("duration") && j["duration"].is_string()) {
        c.duration = j["duration"].get<std::string>();
    }
    if (j.contains("description") && j["description"].is_string()) {
        c.description = j["description"].get<std::string>();
    }
    return c;
}

bool Course::validate(std::string& error_msg) const {
    if (course_name.empty()) {
        error_msg = "Course name is required.";
        return false;
    }
    if (department.empty()) {
        error_msg = "Department is required.";
        return false;
    }
    if (duration.empty()) {
        error_msg = "Duration is required (e.g. '4 Years', '2 Semesters').";
        return false;
    }
    return true;
}
