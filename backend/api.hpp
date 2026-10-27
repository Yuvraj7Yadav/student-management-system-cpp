#pragma once
#include <crow_all.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "database.hpp"

inline std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

class CorsManager {
public:
    CorsManager() {
        refresh();
    }

    void refresh() {
        allowed_origins_.clear();
        const char* env_origins = std::getenv("CORS_ALLOWED_ORIGINS");
        if (env_origins && *env_origins) {
            std::string str(env_origins);
            std::stringstream ss(str);
            std::string item;
            while (std::getline(ss, item, ',')) {
                auto start = item.find_first_not_of(" \t\r\n");
                auto end = item.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    allowed_origins_.push_back(item.substr(start, end - start + 1));
                }
            }
        }
    }

    std::string get_primary_origin() const {
        if (!allowed_origins_.empty()) {
            return allowed_origins_[0];
        }
        return "*";
    }

    bool is_origin_allowed(const std::string& origin) const {
        if (origin.empty()) return true;

        // Check explicitly allowed origins from environment variable
        for (const auto& allowed : allowed_origins_) {
            if (allowed == "*" || allowed == origin) return true;
        }

        // Always allow localhost and loopback for local development
        if (origin == "http://localhost" || origin.rfind("http://localhost:", 0) == 0 ||
            origin == "https://localhost" || origin.rfind("https://localhost:", 0) == 0 ||
            origin == "http://127.0.0.1" || origin.rfind("http://127.0.0.1:", 0) == 0 ||
            origin == "https://127.0.0.1" || origin.rfind("https://127.0.0.1:", 0) == 0) {
            return true;
        }

        // If no explicit list is configured, allow *.vercel.app for seamless out-of-the-box Vercel deployments
        if (allowed_origins_.empty()) {
            if (origin.rfind("https://", 0) == 0 && origin.find(".vercel.app") != std::string::npos) {
                return true;
            }
        }

        return false;
    }

    void apply_headers(const crow::request& req, crow::response& res) const {
        std::string origin = req.get_header_value("Origin");
        if (!origin.empty()) {
            if (is_origin_allowed(origin)) {
                res.add_header("Access-Control-Allow-Origin", origin);
                res.add_header("Vary", "Origin");
                res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Accept");
                res.add_header("Access-Control-Max-Age", "86400");
            }
        } else {
            if (req.method == crow::HTTPMethod::Options || res.code == 204) {
                std::string fallback = get_primary_origin();
                if (!fallback.empty()) {
                    res.add_header("Access-Control-Allow-Origin", fallback);
                }
                res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Accept");
                res.add_header("Access-Control-Max-Age", "86400");
            }
        }
    }

private:
    std::vector<std::string> allowed_origins_;
};

struct DynamicCorsMiddleware {
    struct context {};

    void before_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/) {}

    void after_handle(crow::request& req, crow::response& res, context& /*ctx*/) {
        cors.apply_headers(req, res);
    }

    CorsManager cors;
};

using AppType = crow::App<DynamicCorsMiddleware>;

inline crow::response json_response(int code, const nlohmann::json& j) {
    crow::response res(code, j.dump());
    res.add_header("Content-Type", "application/json; charset=utf-8");
    return res;
}

inline crow::response error_response(int code, const std::string& message) {
    nlohmann::json j = {
        {"success", false},
        {"error", message}
    };
    return json_response(code, j);
}

inline crow::response success_response(int code, const std::string& message, const nlohmann::json& data = nlohmann::json::object()) {
    nlohmann::json j = {
        {"success", true},
        {"message", message}
    };
    for (auto it = data.begin(); it != data.end(); ++it) {
        j[it.key()] = it.value();
    }
    return json_response(code, j);
}

inline void register_routes(AppType& app, Database& db, const std::string& frontend_dir, bool serve_frontend) {
    // ----------------------------------------------------
    // STATIC FRONTEND ROUTES (When enabled / assets present)
    // ----------------------------------------------------
    if (serve_frontend) {
        CROW_ROUTE(app, "/")
        ([frontend_dir]() {
            std::string content = read_file(frontend_dir + "/index.html");
            if (content.empty()) return crow::response(404, "Frontend index.html not found.");
            crow::response res(200, content);
            res.add_header("Content-Type", "text/html; charset=utf-8");
            return res;
        });

        CROW_ROUTE(app, "/index.html")
        ([frontend_dir]() {
            std::string content = read_file(frontend_dir + "/index.html");
            if (content.empty()) return crow::response(404, "Frontend index.html not found.");
            crow::response res(200, content);
            res.add_header("Content-Type", "text/html; charset=utf-8");
            return res;
        });

        CROW_ROUTE(app, "/css/<string>")
        ([frontend_dir](const std::string& filename) {
            std::string content = read_file(frontend_dir + "/css/" + filename);
            if (content.empty()) return crow::response(404, "CSS asset not found.");
            crow::response res(200, content);
            res.add_header("Content-Type", "text/css; charset=utf-8");
            return res;
        });

        CROW_ROUTE(app, "/js/<string>")
        ([frontend_dir](const std::string& filename) {
            std::string content = read_file(frontend_dir + "/js/" + filename);
            if (content.empty()) return crow::response(404, "JavaScript asset not found.");
            crow::response res(200, content);
            res.add_header("Content-Type", "application/javascript; charset=utf-8");
            return res;
        });

        CROW_ROUTE(app, "/favicon.ico")
        ([]() {
            crow::response res(204);
            return res;
        });
    } else {
        // API-only mode: root gives a helpful service info response
        CROW_ROUTE(app, "/")
        ([]() {
            nlohmann::json j = {
                {"status", "online"},
                {"service", "Student Management System API (C++ Crow)"},
                {"version", "1.0.0"},
                {"mode", "API-only (Frontend hosted on Vercel)"},
                {"health_check", "/api/health"},
                {"documentation", "Refer to README.md for endpoint specifications"}
            };
            return json_response(200, j);
        });
    }

    // ----------------------------------------------------
    // REST API ENDPOINTS
    // ----------------------------------------------------

    // 1. Health check
    CROW_ROUTE(app, "/api/health")
    .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        nlohmann::json j = {
            {"status", "ok"},
            {"service", "Student Management System C++ Backend"},
            {"version", "1.0.0"},
            {"engine", "Crow C++ / SQLite3"},
            {"database", "connected"}
        };
        return json_response(200, j);
    });

    // 2. Dashboard Statistics
    CROW_ROUTE(app, "/api/dashboard/stats")
    .methods("GET"_method, "OPTIONS"_method)
    ([&db](const crow::request& req) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        nlohmann::json stats = db.get_dashboard_stats();
        stats["success"] = true;
        return json_response(200, stats);
    });

    // 3. Courses API
    CROW_ROUTE(app, "/api/courses")
    .methods("GET"_method, "POST"_method, "OPTIONS"_method)
    ([&db](const crow::request& req) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        if (req.method == "POST"_method) {
            try {
                auto j = nlohmann::json::parse(req.body);
                Course c = Course::from_json(j);
                std::string err;
                if (!c.validate(err)) {
                    return error_response(400, err);
                }
                if (!db.add_course(c, err)) {
                    return error_response(409, err);
                }
                return success_response(201, "Course added successfully.");
            } catch (const std::exception& e) {
                return error_response(400, "Invalid JSON payload: " + std::string(e.what()));
            }
        }
        // GET method
        auto courses = db.get_all_courses();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& c : courses) {
            arr.push_back(c.to_json());
        }
        nlohmann::json res = {
            {"success", true},
            {"total", courses.size()},
            {"courses", arr}
        };
        return json_response(200, res);
    });

    CROW_ROUTE(app, "/api/courses/<int>")
    .methods("GET"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
    ([&db](const crow::request& req, int id) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        if (req.method == "GET"_method) {
            auto c = db.get_course_by_id(id);
            if (!c.has_value()) {
                return error_response(404, "Course not found.");
            }
            nlohmann::json res = {
                {"success", true},
                {"course", c->to_json()}
            };
            return json_response(200, res);
        }
        if (req.method == "PUT"_method) {
            try {
                auto j = nlohmann::json::parse(req.body);
                Course c = Course::from_json(j);
                std::string err;
                if (!c.validate(err)) {
                    return error_response(400, err);
                }
                if (!db.update_course(id, c, err)) {
                    return error_response(400, err);
                }
                return success_response(200, "Course updated successfully.");
            } catch (const std::exception& e) {
                return error_response(400, "Invalid JSON payload: " + std::string(e.what()));
            }
        }
        if (req.method == "DELETE"_method) {
            std::string err;
            if (!db.delete_course(id, err)) {
                return error_response(400, err);
            }
            return success_response(200, "Course deleted successfully.");
        }
        return error_response(405, "Method Not Allowed");
    });

    // 4. Students API
    CROW_ROUTE(app, "/api/students")
    .methods("GET"_method, "POST"_method, "OPTIONS"_method)
    ([&db](const crow::request& req) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        if (req.method == "POST"_method) {
            try {
                auto j = nlohmann::json::parse(req.body);
                Student s = Student::from_json(j);
                std::string err;
                if (!s.validate(err)) {
                    return error_response(400, err);
                }
                if (!db.add_student(s, err)) {
                    return error_response(409, err);
                }
                nlohmann::json data = {
                    {"student", s.to_json()}
                };
                return success_response(201, "Student created successfully.", data);
            } catch (const std::exception& e) {
                return error_response(400, "Invalid JSON payload: " + std::string(e.what()));
            }
        }
        // GET method
        std::string search = req.url_params.get("search") ? req.url_params.get("search") : "";
        std::string course_str = req.url_params.get("course_id") ? req.url_params.get("course_id") : "0";
        std::string grade = req.url_params.get("grade") ? req.url_params.get("grade") : "";
        std::string sort_by = req.url_params.get("sort_by") ? req.url_params.get("sort_by") : "id";
        std::string sort_dir = req.url_params.get("sort_dir") ? req.url_params.get("sort_dir") : "desc";
        std::string page_str = req.url_params.get("page") ? req.url_params.get("page") : "1";
        std::string limit_str = req.url_params.get("limit") ? req.url_params.get("limit") : "10";

        int course_id = 0;
        int page = 1;
        int limit = 10;
        try { course_id = std::stoi(course_str); } catch (...) {}
        try { page = std::max(1, std::stoi(page_str)); } catch (...) {}
        try { limit = std::max(0, std::stoi(limit_str)); } catch (...) {}

        int total_count = 0;
        auto students = db.get_students(search, course_id, grade, sort_by, sort_dir, page, limit, total_count);

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& s : students) {
            arr.push_back(s.to_json());
        }

        int total_pages = (limit > 0) ? (total_count + limit - 1) / limit : 1;

        nlohmann::json res = {
            {"success", true},
            {"total", total_count},
            {"page", page},
            {"limit", limit},
            {"total_pages", total_pages},
            {"students", arr}
        };
        return json_response(200, res);
    });

    CROW_ROUTE(app, "/api/students/<int>")
    .methods("GET"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
    ([&db](const crow::request& req, int id) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        if (req.method == "GET"_method) {
            auto s = db.get_student_by_id(id);
            if (!s.has_value()) {
                return error_response(404, "Student not found.");
            }
            nlohmann::json res = {
                {"success", true},
                {"student", s->to_json()}
            };
            return json_response(200, res);
        }
        if (req.method == "PUT"_method) {
            try {
                auto j = nlohmann::json::parse(req.body);
                Student s = Student::from_json(j);
                std::string err;
                if (!s.validate(err)) {
                    return error_response(400, err);
                }
                if (!db.update_student(id, s, err)) {
                    return error_response(400, err);
                }
                nlohmann::json data = {
                    {"student", s.to_json()}
                };
                return success_response(200, "Student updated successfully.", data);
            } catch (const std::exception& e) {
                return error_response(400, "Invalid JSON payload: " + std::string(e.what()));
            }
        }
        if (req.method == "DELETE"_method) {
            std::string err;
            if (!db.delete_student(id, err)) {
                return error_response(404, err);
            }
            return success_response(200, "Student deleted successfully.");
        }
        return error_response(405, "Method Not Allowed");
    });

    // 5. Results API
    CROW_ROUTE(app, "/api/results")
    .methods("GET"_method, "OPTIONS"_method)
    ([&db](const crow::request& req) {
        if (req.method == "OPTIONS"_method) {
            return crow::response(204);
        }
        std::string course_str = req.url_params.get("course_id") ? req.url_params.get("course_id") : "0";
        std::string grade = req.url_params.get("grade") ? req.url_params.get("grade") : "";
        std::string status = req.url_params.get("status") ? req.url_params.get("status") : "";
        std::string search = req.url_params.get("search") ? req.url_params.get("search") : "";

        int course_id = 0;
        try { course_id = std::stoi(course_str); } catch (...) {}

        nlohmann::json res = db.get_results(course_id, grade, status, search);
        res["success"] = true;
        return json_response(200, res);
    });
}
