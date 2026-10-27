# EduTrack - College Student Management System

A high-performance, modern, full-stack **Student Management System** built with a pure **C++17** backend utilizing the **Crow** web framework, **SQLite3** database, and a responsive administrative dashboard frontend built with **HTML5, CSS3, and vanilla JavaScript**.

The C++ backend directly handles all business logic, parameterized SQL operations, input validations, grade/percentage calculations, and static asset serving. **No Node.js, Python, Java, or PHP runtime is required.**

---

## Table of Contents

1. [Key Features](#key-features)
2. [Technology Stack](#technology-stack)
3. [Database Architecture & Schema](#database-architecture--schema)
4. [Project Directory Structure](#project-directory-structure)
5. [Prerequisites on Windows](#prerequisites-on-windows)
6. [Step-by-Step Build & Run Instructions](#step-by-step-build--run-instructions)
7. [API Endpoint Documentation](#api-endpoint-documentation)
8. [Grading Scheme & Business Logic](#grading-scheme--business-logic)
9. [Troubleshooting Guide](#troubleshooting-guide)
10. [Production Deployment Guide (Vercel + Render)](#10-production-deployment-guide-vercel--render)

---

## 1. Key Features

- **Interactive Admin Dashboard:**
  - Dynamic KPI cards displaying: Total Students, Active Courses, Average Marks, Passing Students, and Failing Students.
  - Live Grade Distribution visual progress chart ($A+$, $A$, $B$, $C$, $D$, $F$).
  - Recently enrolled students table with quick profile review links.
  - Quick action shortcuts to enroll students and create courses.

- **Complete Student CRUD Management:**
  - Add, View, Edit, and Delete student records with confirmation dialogs.
  - Real-time search across Student ID, Full Name, and Email address.
  - Multi-criteria filtering by Course and Letter Grade.
  - Dynamic column sorting (by ID, Name, Marks, Grade) and pagination.
  - Detailed Student Profile view displaying full personal and academic records.

- **Course & Academic Program Management:**
  - Add, edit, and delete academic courses and college departments.
  - Live student enrollment counter per course.
  - **Referential Integrity Protection:** Prevents deletion of any course program that currently has enrolled students.

- **Results & Examination Ledger:**
  - Overall class analytics: Total examinees, passed, failed, pass rate percentage, and average percentage.
  - Multi-parameter filtering by Course, Grade, Pass/Fail status, and Name/ID search.

- **Robust Security & Validation:**
  - Parameterized SQLite prepared statements (`sqlite3_prepare_v2` / `sqlite3_bind_*`) protecting against SQL injection.
  - Unique constraint enforcement for Student IDs and Email addresses.
  - Strict validation of marks ($0 \le \text{marks} \le \text{maximum marks}$), semesters ($1 \le \text{sem} \le 12$), and email formats.
  - Toast notification system and friendly empty/loading states.

---

## 2. Technology Stack

| Layer | Technology | Details |
|---|---|---|
| **Backend Language** | C++17 | Core business logic, grade calculations, database transactions |
| **Web Framework** | Crow v1.2.0 | Microframework with Asio standalone networking |
| **Database** | SQLite 3.47 | Embedded relational database with Foreign Keys & WAL mode |
| **JSON Library** | `nlohmann/json` (v3.11.3) | Modern C++ JSON serialization and parsing |
| **Build System** | CMake 3.15+ | Cross-platform build automation |
| **Frontend** | HTML5, CSS3, Vanilla JS | Modern responsive dashboard, no heavy frontend frameworks needed |
| **Web Server** | Embedded in C++ | Serves frontend static assets and REST API at `http://localhost:18080` |

---

## 3. Database Architecture & Schema

The SQLite database file is created automatically on application startup at `data/student_management.db`. Foreign key constraints (`PRAGMA foreign_keys = ON;`) and Write-Ahead Logging (`PRAGMA journal_mode = WAL;`) are enforced.

```sql
-- 1. Courses Table
CREATE TABLE IF NOT EXISTS courses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    course_name TEXT UNIQUE NOT NULL,
    department TEXT NOT NULL,
    duration TEXT NOT NULL,
    description TEXT
);

-- 2. Students Table
CREATE TABLE IF NOT EXISTS students (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id TEXT UNIQUE NOT NULL,
    full_name TEXT NOT NULL,
    email TEXT UNIQUE NOT NULL,
    phone TEXT,
    course_id INTEGER NOT NULL,
    semester INTEGER NOT NULL,
    marks_obtained REAL NOT NULL,
    maximum_marks REAL NOT NULL,
    percentage REAL NOT NULL,
    grade TEXT NOT NULL,
    status TEXT NOT NULL,
    admission_date TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (course_id) REFERENCES courses(id) ON DELETE RESTRICT
);

-- 3. Indexes for fast query lookup
CREATE INDEX IF NOT EXISTS idx_student_id ON students(student_id);
CREATE INDEX IF NOT EXISTS idx_student_email ON students(email);
CREATE INDEX IF NOT EXISTS idx_student_course ON students(course_id);
```

---

## 4. Project Directory Structure

```
student-management-system/
├── CMakeLists.txt              # CMake configuration and build directives
├── README.md                   # Complete documentation and setup guide
├── .gitignore                  # Git ignore rules for build artifacts and DB journals
├── backend/
│   ├── main.cpp                # Server entry point, CLI arguments, static serving
│   ├── api.hpp                 # Crow route definitions and REST controllers
│   ├── database.hpp            # SQLite Database class interface and thread safety
│   ├── database.cpp            # Prepared statements, transactions, queries, seeds
│   ├── student.hpp             # Student model, serialization, validations
│   ├── student.cpp             # Grade and percentage calculations
│   ├── course.hpp              # Course model
│   └── course.cpp              # Course validation and serialization
├── frontend/
│   ├── index.html              # Responsive dashboard UI layout
│   ├── css/
│   │   └── style.css           # Blue/gray professional dashboard stylesheet
│   ├── js/
│   │   └── app.js              # Vanilla JavaScript API client and DOM management
│   └── assets/                 # Icons and static web graphics
├── third_party/                # Self-contained header dependencies (no external installs)
│   ├── asio/                   # Standalone Asio headers
│   ├── crow/                   # Crow C++ single-header
│   ├── nlohmann/               # nlohmann/json headers
│   └── sqlite/                 # SQLite3 C amalgamation (sqlite3.c & sqlite3.h)
└── data/
    └── student_management.db   # Auto-created persistent SQLite database
```

---

## 5. Prerequisites on Windows

### A. C++ Compiler (MinGW-w64 GCC or MSVC)
A modern C++17 compiler is required:
- **MinGW-w64 (Recommended):** GCC 9+ (e.g. GCC 14/16 from WinLibs or MSYS2).
- **Or Visual Studio 2022:** With the "Desktop development with C++" workload.

To install MinGW-w64 on Windows via Windows Package Manager:
```powershell
winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e
```
Ensure `g++.exe` is in your environment `PATH`:
```powershell
g++ --version
```

### B. CMake
Ensure CMake (v3.15 or newer) is installed and in `PATH`:
```powershell
cmake --version
```
If CMake is not installed:
```powershell
winget install --id Kitware.CMake -e
```

---

## 6. Step-by-Step Build & Run Instructions

### Step 1: Open PowerShell or Command Prompt
Open a terminal inside the project root folder:
```powershell
cd student_management_system_cpp
```

### Step 2: Configure the Project with CMake
Generate the build system using Ninja (or MinGW Makefiles):
```powershell
cmake -S . -B build -G "Ninja"
```
*(If you do not have Ninja installed, run `cmake -S . -B build -G "MinGW Makefiles"` instead).*

### Step 3: Compile the C++ Server
Build the executable:
```powershell
cmake --build build
```
This produces `build\student_management_server.exe`.

### Step 4: Run the Application
Start the backend server:
```powershell
.\build\student_management_server.exe
```
Output:
```text
========================================================
     COLLEGE STUDENT MANAGEMENT SYSTEM (C++ BACKEND)    
========================================================
[INFO] Initializing SQLite database at: data/student_management.db
[INFO] Serving frontend assets from:    frontend
[SUCCESS] Database initialized and verified successfully.
--------------------------------------------------------
[SERVER RUNNING] Web server started on port 18080
[ACCESS URL]     http://localhost:18080
[API HEALTH]     http://localhost:18080/api/health
--------------------------------------------------------
Press Ctrl+C to stop the server.
```

### Step 5: Open the Application in your Browser
Navigate to:
```
http://localhost:18080
```
The entire application is live! You can view statistics, create/edit/delete students, add courses, and inspect grades.

---

## 7. API Endpoint Documentation

All endpoints return JSON responses and support CORS.

### Health Check
- **`GET /api/health`**
  - Response:
    ```json
    {
      "engine": "Crow C++ / SQLite3",
      "service": "Student Management System C++ Backend",
      "status": "ok",
      "version": "1.0.0"
    }
    ```

### Dashboard Statistics
- **`GET /api/dashboard/stats`**
  - Returns real-time aggregate statistics:
    ```json
    {
      "success": true,
      "total_students": 8,
      "total_courses": 5,
      "average_marks": 361.0,
      "average_percentage": 72.2,
      "passing_count": 7,
      "failing_count": 1,
      "grade_distribution": { "A+": 2, "A": 1, "B": 2, "C": 1, "D": 1, "F": 1 },
      "recent_students": [ ... ]
    }
    ```

### Student Endpoints
- **`GET /api/students`**
  - Query parameters:
    - `search` (string): Search in student ID, name, or email.
    - `course_id` (int): Filter by specific course ID.
    - `grade` (string): Filter by letter grade (`A+`, `A`, `B`, `C`, `D`, `F`).
    - `sort_by` (string): `id`, `student_id`, `full_name`, `marks_obtained`, `percentage`, `admission_date`.
    - `sort_dir` (string): `asc` or `desc`.
    - `page` (int, default: 1): Current page.
    - `limit` (int, default: 10): Records per page.
- **`POST /api/students`**
  - Request body:
    ```json
    {
      "student_id": "STU2024101",
      "full_name": "Rohan Gupta",
      "email": "rohan.g@example.edu",
      "phone": "+91 9876501234",
      "course_id": 1,
      "semester": 3,
      "marks_obtained": 430.0,
      "maximum_marks": 500.0,
      "admission_date": "2023-08-10"
    }
    ```
  - Calculates percentage, grade, and status in the C++ backend.
  - Returns HTTP 201 on success, HTTP 400 on invalid input, HTTP 409 on duplicate ID/email.
- **`GET /api/students/<id>`**
  - Retrieves individual student record with joined course details.
- **`PUT /api/students/<id>`**
  - Updates student information and automatically recalculates academic performance in the C++ backend.
- **`DELETE /api/students/<id>`**
  - Permanently removes a student record.

### Course Endpoints
- **`GET /api/courses`**
  - Lists all courses along with enrolled student count.
- **`POST /api/courses`**
  - Creates a new academic degree course program.
- **`PUT /api/courses/<id>`**
  - Updates course name, department, duration, or description.
- **`DELETE /api/courses/<id>`**
  - Deletes course only if zero students are enrolled. Rejects with HTTP 400 if active students are assigned.

### Results Ledger
- **`GET /api/results`**
  - Query parameters: `course_id`, `grade`, `status` (`Pass`/`Fail`), `search`.
  - Returns ranking ledger with summary metrics: `total`, `passed`, `failed`, `pass_rate`, `average_percentage`.

---

## 8. Grading Scheme & Business Logic

Grades and pass/fail statuses are computed strictly in C++ (`Student::calculate_grade_and_status()`) using:

$$\text{Percentage} = \left(\frac{\text{Marks Obtained}}{\text{Maximum Marks}}\right) \times 100$$

| Percentage Range | Grade | Result Status |
|---|---|---|
| **90.00% – 100.00%** | **A+** | **Pass** |
| **80.00% – 89.99%** | **A** | **Pass** |
| **70.00% – 79.99%** | **B** | **Pass** |
| **60.00% – 69.99%** | **C** | **Pass** |
| **40.00% – 59.99%** | **D** | **Pass** |
| **Below 40.00%** | **F** | **Fail** |

- **Passing Threshold:** 40.00%.
- **Validation Rules:**
  - $\text{Marks Obtained} \ge 0.0$
  - $\text{Maximum Marks} > 0.0$
  - $\text{Marks Obtained} \le \text{Maximum Marks}$
  - Division by zero is safeguarded (returns 0% and Grade F).

---

## 9. Troubleshooting Guide

### 1. `fatal error: asio.hpp: No such file or directory`
All third-party libraries (`crow`, `asio`, `nlohmann`, `sqlite`) are included directly in the `third_party/` directory of this repository. Run CMake configuration from the repository root:
```powershell
cmake -S . -B build -G "Ninja"
```

### 2. `missing libgcc_s_seh-1.dll or libstdc++-6.dll`
Ensure the MinGW `bin` folder (e.g. `C:\mingw64\bin`) is in your environment `PATH`, or run the server from a command prompt with MinGW initialized:
```powershell
$env:Path = "C:\mingw64\bin;$env:Path"
.\build\student_management_server.exe
```

### 3. Port `18080` Already in Use
You can specify a different port as a CLI argument:
```powershell
.\build\student_management_server.exe 18090
```
Then access `http://localhost:18090` in your browser.

### 4. Custom Database Location
You can pass custom paths for the database and frontend directory:
```powershell
.\build\student_management_server.exe 18080 "data/custom_db.db" "frontend"
```

---

## 10. Production Deployment Guide (Vercel + Render)

The system is architected for dual-cloud deployment:
- **Frontend:** Hosted on **Vercel** as high-speed static assets (HTML5, CSS3, Vanilla JS).
- **Backend:** Hosted on **Render** as a containerized Linux C++17 web service with Crow.
- **Database:** SQLite3 database saved on a **Render Persistent Disk** mounted at `/app/data`.

```
                    ┌──────────────────────────┐
                    │    Vercel CDN Edge       │
                    │   (frontend/ static)     │
                    └────────────┬─────────────┘
                                 │
             REST API Calls via HTTPS (CORS protected)
                                 │
                                 ▼
                    ┌──────────────────────────┐
                    │    Render Web Service    │
                    │   (Docker C++ Crow App)  │
                    └────────────┬─────────────┘
                                 │
                         Embedded Transactions
                                 │
                                 ▼
                    ┌──────────────────────────┐
                    │  Render Persistent Disk  │
                    │  (/app/data/*.db SQLite) │
                    └──────────────────────────┘
```

---

### Step 1: Push Repository to GitHub

Ensure all source files, `Dockerfile`, `render.yaml`, `vercel.json`, and frontend assets are committed:

```bash
git init
git add .
git commit -m "feat: complete student management system with vercel and render configs"
git branch -M main
git remote add origin https://github.com/<your-username>/<your-repo-name>.git
git push -u origin main
```

---

### Step 2: Deploy C++ Backend on Render

1. Log in to [Render.com](https://render.com) and navigate to **Dashboard**.
2. Click **New +** > **Web Service**.
3. Connect your GitHub repository.
4. Configure the Web Service:
   - **Name:** `student-management-api` (or preferred name).
   - **Region:** Closest to your users (e.g., *Singapore*, *Oregon*, *Frankfurt*).
   - **Runtime:** **Docker** (Render will automatically detect the root `Dockerfile`).
   - **Plan:** Free or Starter.
5. **Environment Variables:**
   Under **Environment Variables**, add:
   | Key | Value | Description |
   |---|---|---|
   | `PORT` | `10000` | Port expected by Render (Render also sets this automatically) |
   | `DB_PATH` | `/app/data/student_management.db` | Persistent SQLite file path |
   | `BIND_ADDRESS` | `0.0.0.0` | Bind address for container networking |
   | `CORS_ALLOWED_ORIGINS` | `https://your-frontend.vercel.app` | Comma-separated list of allowed frontend URLs (or leave blank to allow all `*.vercel.app`) |

6. **Attach Persistent Disk (Crucial for Data Persistence):**
   - In the **Disks** tab of your Render service, click **Add Disk**.
   - **Name:** `sqlite-data`
   - **Mount Path:** `/app/data`
   - **Size:** `1 GB` (sufficient for hundreds of thousands of student records).
7. Click **Create Web Service**.
8. Once built and deployed, test your live health check endpoint in your browser or curl:
   ```bash
   curl https://<your-render-app>.onrender.com/api/health
   ```
   Expected response:
   ```json
   {"database":"connected","engine":"Crow C++ / SQLite3","service":"Student Management System C++ Backend","status":"ok","version":"1.0.0"}
   ```

---

### Step 3: Deploy Frontend on Vercel

1. Log in to [Vercel.com](https://vercel.com) and click **Add New...** > **Project**.
2. Import the same GitHub repository.
3. Configure the Project:
   - **Framework Preset:** **Other**
   - **Root Directory:** `./`
   - **Build Command:** *(Leave empty)*
   - **Output Directory:** `frontend` *(already defined in `vercel.json`)*
4. Click **Deploy**.
5. Your frontend will be assigned a live Vercel domain:
   `https://<project-name>.vercel.app`

---

### Step 4: Connect Frontend to Backend

You have two convenient methods to configure the backend API URL:

#### Method A: Direct File Configuration (Recommended for Production)
In [frontend/js/config.js](file:///frontend/js/config.js), edit `API_BASE`:
```javascript
window.EDUTRACK_CONFIG = {
    // Set to your deployed Render URL without trailing slash:
    API_BASE: "https://<your-render-app>.onrender.com"
};
```
Commit and push the change to GitHub, and Vercel will instantly redeploy.

#### Method B: Interactive In-App Settings Modal (Zero Redeploy)
1. Open your live Vercel website in the browser.
2. In the lower-left sidebar footer, click the **Settings (gear icon)** button next to the backend status badge.
3. In the modal, paste your Render backend URL:
   `https://<your-render-app>.onrender.com`
4. Click **Save & Connect**. The dashboard will instantly reconnect to the Render C++ backend, display a green **C++ Server Online** status, and persist the setting in `localStorage`.

---

### Step 5: Production Verification Checklist

- [ ] **Health Endpoint:** Verify `GET /api/health` returns `200 OK` with database connected.
- [ ] **CORS Preflight:** Verify preflight `OPTIONS` requests from your Vercel domain succeed with `204 No Content` and `Access-Control-Allow-Origin`.
- [ ] **Data Persistence:** Create a student and a course, restart the Render web service, and verify the records remain intact from the persistent disk.
- [ ] **Asset Security:** Confirm that no backend C++ source code (`backend/`, `CMakeLists.txt`, `Dockerfile`) is publicly accessible via the Vercel site.
