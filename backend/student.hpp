#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct Student {
    int id{0};
    std::string student_id;
    std::string full_name;
    std::string email;
    std::string phone;
    int course_id{0};
    std::string course_name;
    std::string department;
    int semester{1};
    double marks_obtained{0.0};
    double maximum_marks{100.0};
    double percentage{0.0};
    std::string grade{"F"};
    std::string status{"Fail"};
    std::string admission_date;
    std::string created_at;
    std::string updated_at;

    void calculate_grade_and_status();
    bool validate(std::string& error_msg) const;
    nlohmann::json to_json() const;
    static Student from_json(const nlohmann::json& j);
};
