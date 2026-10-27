#define CROW_MAIN
#include <crow_all.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <exception>
#include <cstdlib>
#include "database.hpp"
#include "api.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Unbuffer stdout and stderr so diagnostics appear immediately
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "========================================================\n";
    std::cout << "     COLLEGE STUDENT MANAGEMENT SYSTEM (C++ BACKEND)    \n";
    std::cout << "========================================================\n";

    try {
        // 1. Resolve Port: ENV("PORT") -> CLI arg -> Default 18080
        int port = 18080;
        const char* env_port = std::getenv("PORT");
        if (env_port && *env_port) {
            try {
                int p = std::stoi(env_port);
                if (p > 0 && p <= 65535) {
                    port = p;
                }
            } catch (...) {
                std::cerr << "[WARNING] Invalid PORT env var '" << env_port << "', using default.\n";
            }
        }
        if (argc > 1) {
            try {
                int p = std::stoi(argv[1]);
                if (p > 0 && p <= 65535) {
                    port = p;
                }
            } catch (const std::exception& e) {
                std::cerr << "[WARNING] Invalid port CLI argument '" << argv[1] 
                          << "', using " << port << " (" << e.what() << ")\n";
            }
        }

        // 2. Resolve Bind Address: ENV("BIND_ADDRESS") -> Default "0.0.0.0"
        std::string bind_addr = "0.0.0.0";
        const char* env_bind = std::getenv("BIND_ADDRESS");
        if (env_bind && *env_bind) {
            bind_addr = env_bind;
        }

        // 3. Resolve Database Path: ENV("DB_PATH") -> CLI arg -> Default "data/student_management.db"
        std::string db_path = "data/student_management.db";
        const char* env_db = std::getenv("DB_PATH");
        if (env_db && *env_db) {
            db_path = env_db;
        }
        if (argc > 2) {
            db_path = argv[2];
        }

        // 4. Resolve Frontend Directory: ENV("FRONTEND_DIR") -> CLI arg -> Default "frontend"
        std::string frontend_dir = "frontend";
        const char* env_fe = std::getenv("FRONTEND_DIR");
        if (env_fe && *env_fe) {
            frontend_dir = env_fe;
        }
        if (argc > 3) {
            frontend_dir = argv[3];
        }

        // Auto-detect frontend directory if running from a build/ subdirectory
        bool serve_frontend = false;
        if (fs::exists(frontend_dir + "/index.html")) {
            serve_frontend = true;
        } else if (fs::exists("../" + frontend_dir + "/index.html")) {
            frontend_dir = "../" + frontend_dir;
            serve_frontend = true;
        } else if (fs::exists("../../" + frontend_dir + "/index.html")) {
            frontend_dir = "../../" + frontend_dir;
            serve_frontend = true;
        }

        if (serve_frontend) {
            std::cout << "[INFO] Serving frontend assets from: " << fs::absolute(frontend_dir).string() << "\n";
        } else {
            std::cout << "[INFO] Frontend directory not found locally. Running in API-only backend mode.\n"
                      << "       (Vercel frontend communicates with this backend over REST API)\n";
        }

        // Auto-detect & ensure parent data directory exists for SQLite
        fs::path p_db(db_path);
        if (p_db.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(p_db.parent_path(), ec);
            if (ec) {
                std::cerr << "[FATAL ERROR] Failed to create database directory: " 
                          << p_db.parent_path().string() << " (" << ec.message() << ")\n";
                return 1;
            }
        }

        std::cout << "[INFO] Initializing SQLite database at: " << fs::absolute(p_db).string() << "\n";

        Database db;
        if (!db.init(db_path)) {
            std::cerr << "[FATAL ERROR] Failed to initialize SQLite database at: " << db_path << "\n";
            return 1;
        }
        std::cout << "[SUCCESS] Database schema initialized and verified successfully.\n";

        // Initialize CORS Manager info
        const char* env_cors = std::getenv("CORS_ALLOWED_ORIGINS");
        if (env_cors && *env_cors) {
            std::cout << "[INFO] CORS allowed origins configured: " << env_cors << "\n";
        } else {
            std::cout << "[INFO] CORS: Allowing localhost and *.vercel.app by default.\n";
        }

        AppType app;
        // Keep console output focused on application logs
        app.loglevel(crow::LogLevel::Warning);

        register_routes(app, db, frontend_dir, serve_frontend);

        std::cout << "--------------------------------------------------------\n";
        std::cout << "[SERVER RUNNING] Web server listening on " << bind_addr << ":" << port << "\n";
        std::cout << "[ACCESS URL]     http://" << (bind_addr == "0.0.0.0" ? "localhost" : bind_addr) << ":" << port << "\n";
        std::cout << "[API HEALTH]     http://" << (bind_addr == "0.0.0.0" ? "localhost" : bind_addr) << ":" << port << "/api/health\n";
        std::cout << "--------------------------------------------------------\n";
        std::cout << "Press Ctrl+C to stop the server.\n\n";

        app.bindaddr(bind_addr).port(port).multithreaded().run();

    } catch (const std::exception& e) {
        std::cerr << "[FATAL EXCEPTION] Server terminated with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[FATAL EXCEPTION] Server terminated with an unknown error.\n";
        return 1;
    }

    return 0;
}
