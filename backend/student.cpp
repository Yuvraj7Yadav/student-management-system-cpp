#include "student.hpp"
#include <cmath>
#include <regex>
#include <sstream>
#include <iomanip>

void Student::calculate_grade_and_status() {
    if (maximum_marks <= 0.0) {
        percentage = 0.0;
        grade = "F";
        status = "Fail";
        return;
    }

    percentage = (marks_obtained / maximum_marks) * 100.0;
    // Round to 2 decimal places
    percentage = std::round(percentage * 100.0) / 100.0;

    if (percentage >= 90.0) {
        grade = "A+";
    } else if (percentage >= 80.0) {
        grade = "A";
    } else if (percentage >= 70.0) {
        grade = "B";
    } else if (percentage >= 60.0) {
        grade = "C";
    } else if (percentage >= 40.0) {
        grade = "D";
    } else {
        grade = "F";
    }

    if (percentage >= 40.0) {
        status = "Pass";
    } else {
        status = "Fail";
    }
}

bool Student::validate(std::string& error_msg) const {
    if (student_id.empty()) {
        error_msg = "Student ID is required.";
        return false;
    }
    if (full_name.empty()) {
        error_msg = "Full name is required.";
        return false;
    }
    if (email.empty()) {
        error_msg = "Email address is required.";
        return false;
    }

    // Basic email check
    static const std::regex email_regex(R"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)");
    if (!std::regex_match(email, email_regex)) {
        error_msg = "Invalid email format.";
        return false;
    }

    if (course_id <= 0) {
        error_msg = "Valid Course selection is required.";
        return false;
    }

    if (semester < 1 || semester > 12) {
        error_msg = "Semester must be between 1 and 12.";
        return false;
    }

    if (maximum_marks <= 0.0) {
        error_msg = "Maximum marks must be greater than zero.";
        return false;
    }

    if (marks_obtained < 0.0) {
        error_msg = "Marks obtained cannot be negative.";
        return false;
    }

    if (marks_obtained > maximum_marks) {
        error_msg = "Marks obtained cannot exceed maximum marks.";
        return false;
    }

    if (admission_date.empty()) {
        error_msg = "Admission date is required.";
        return false;
    }

    return true;
}

nlohmann::json Student::to_json() const {
    return nlohmann::json{
        {"id", id},
        {"student_id", student_id},
        {"full_name", full_name},
        {"email", email},
        {"phone", phone},
        {"course_id", course_id},
        {"course_name", course_name},
        {"department", department},
        {"semester", semester},
        {"marks_obtained", marks_obtained},
        {"maximum_marks", maximum_marks},
        {"percentage", percentage},
        {"grade", grade},
        {"status", status},
        {"admission_date", admission_date},
        {"created_at", created_at},
        {"updated_at", updated_at}
    };
}

Student Student::from_json(const nlohmann::json& j) {
    Student s;
    if (j.contains("id") && j["id"].is_number_integer()) {
        s.id = j["id"].get<int>();
    }
    if (j.contains("student_id") && j["student_id"].is_string()) {
        s.student_id = j["student_id"].get<std::string>();
    }
    if (j.contains("full_name") && j["full_name"].is_string()) {
        s.full_name = j["full_name"].get<std::string>();
    }
    if (j.contains("email") && j["email"].is_string()) {
        s.email = j["email"].get<std::string>();
    }
    if (j.contains("phone") && j["phone"].is_string()) {
        s.phone = j["phone"].get<std::string>();
    }
    if (j.contains("course_id") && j["course_id"].is_number_integer()) {
        s.course_id = j["course_id"].get<int>();
    }
    if (j.contains("semester") && j["semester"].is_number_integer()) {
        s.semester = j["semester"].get<int>();
    }
    if (j.contains("marks_obtained") && j["marks_obtained"].is_number()) {
        s.marks_obtained = j["marks_obtained"].get<double>();
    }
    if (j.contains("maximum_marks") && j["maximum_marks"].is_number()) {
        s.maximum_marks = j["maximum_marks"].get<double>();
    }
    if (j.contains("admission_date") && j["admission_date"].is_string()) {
        s.admission_date = j["admission_date"].get<std::string>();
    }
    return s;
}
