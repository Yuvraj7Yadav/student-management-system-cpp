#pragma once
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include "course.hpp"
#include "student.hpp"

class Database {
public:
    Database();
    ~Database();

    bool init(const std::string& db_file_path);
    void close();

    // Course CRUD
    std::vector<Course> get_all_courses();
    std::optional<Course> get_course_by_id(int id);
    bool add_course(const Course& course, std::string& error_msg);
    bool update_course(int id, const Course& course, std::string& error_msg);
    bool delete_course(int id, std::string& error_msg);

    // Student CRUD
    std::vector<Student> get_students(
        const std::string& search,
        int course_id,
        const std::string& grade,
        const std::string& sort_by,
        const std::string& sort_dir,
        int page,
        int limit,
        int& total_count
    );
    std::optional<Student> get_student_by_id(int id);
    bool add_student(Student& student, std::string& error_msg);
    bool update_student(int id, Student& student, std::string& error_msg);
    bool delete_student(int id, std::string& error_msg);

    // Analytics and Results
    nlohmann::json get_dashboard_stats();
    nlohmann::json get_results(int course_id, const std::string& grade, const std::string& status, const std::string& search);

private:
    sqlite3* db_{nullptr};
    std::mutex db_mutex_;

    bool create_tables();
    void seed_default_data();
    static std::string current_timestamp();
};
