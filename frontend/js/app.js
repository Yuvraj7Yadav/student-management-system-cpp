/**
 * EduTrack - College Student Management System Frontend
 * Connects directly to the Crow C++ REST API & SQLite Backend
 */

// Application State
const state = {
    activeTab: 'dashboard',
    courses: [],
    students: [],
    studentPagination: {
        page: 1,
        limit: 10,
        total: 0,
        totalPages: 1
    },
    studentFilters: {
        search: '',
        courseId: 0,
        grade: '',
        sortBy: 'id',
        sortDir: 'desc'
    },
    resultsFilters: {
        search: '',
        courseId: 0,
        grade: '',
        status: ''
    },
    activeStudentProfileId: null
};

// Resolve API Base URL with prioritized fallback:
// 1. localStorage override ('EDUTRACK_API_BASE')
// 2. window.EDUTRACK_CONFIG.API_BASE (from /js/config.js)
// 3. window.API_BASE
// 4. Default to empty string '' (local relative path for development)
function resolveApiBase() {
    try {
        const stored = localStorage.getItem('EDUTRACK_API_BASE');
        if (stored && stored.trim() !== '') {
            return stored.trim().replace(/\/+$/, '');
        }
    } catch (_) {}

    if (window.EDUTRACK_CONFIG && typeof window.EDUTRACK_CONFIG.API_BASE === 'string' && window.EDUTRACK_CONFIG.API_BASE.trim() !== '') {
        return window.EDUTRACK_CONFIG.API_BASE.trim().replace(/\/+$/, '');
    }

    if (typeof window.API_BASE === 'string' && window.API_BASE.trim() !== '') {
        return window.API_BASE.trim().replace(/\/+$/, '');
    }

    return '';
}

let API_BASE = resolveApiBase();

// ==========================================
// TOAST NOTIFICATIONS
// ==========================================
function showToast(message, type = 'info', duration = 4000) {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;

    let icon = 'ℹ️';
    if (type === 'success') icon = '✅';
    else if (type === 'error') icon = '❌';
    else if (type === 'warning') icon = '⚠️';

    toast.innerHTML = `
        <span style="font-size: 16px;">${icon}</span>
        <div style="flex: 1; font-weight: 500;">${escapeHtml(message)}</div>
    `;

    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        setTimeout(() => toast.remove(), 250);
    }, duration);
}

// Helper: Escape HTML
function escapeHtml(str) {
    if (str === null || str === undefined) return '';
    return String(str)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#039;');
}

// Helper: Format Date
function formatDate(dateStr) {
    if (!dateStr) return 'N/A';
    try {
        const d = new Date(dateStr);
        if (isNaN(d.getTime())) return dateStr;
        return d.toLocaleDateString(undefined, { year: 'numeric', month: 'short', day: 'numeric' });
    } catch {
        return dateStr;
    }
}

// Helper: Grade Badge Class
function getGradeBadgeClass(grade) {
    switch (grade) {
        case 'A+': return 'badge-grade-aplus';
        case 'A': return 'badge-grade-a';
        case 'B': return 'badge-grade-b';
        case 'C': return 'badge-grade-c';
        case 'D': return 'badge-grade-d';
        case 'F': return 'badge-grade-f';
        default: return 'badge-grade-f';
    }
}

// ==========================================
// API CLIENT
// ==========================================
async function apiRequest(endpoint, method = 'GET', data = null) {
    const options = {
        method,
        headers: {
            'Accept': 'application/json'
        }
    };

    if (data && (method === 'POST' || method === 'PUT')) {
        options.headers['Content-Type'] = 'application/json';
        options.body = JSON.stringify(data);
    }

    try {
        const response = await fetch(`${API_BASE}${endpoint}`, options);
        const result = await response.json();
        if (!response.ok) {
            throw new Error(result.error || `Request failed with status ${response.status}`);
        }
        return result;
    } catch (err) {
        console.error(`API Error on ${endpoint}:`, err);
        throw err;
    }
}

// ==========================================
// MODAL MANAGEMENT
// ==========================================
function openModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) modal.classList.add('active');
}

function closeModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.remove('active');
        const form = modal.querySelector('form');
        if (form) {
            form.reset();
            modal.querySelectorAll('.field-error-msg').forEach(el => el.classList.remove('visible'));
            modal.querySelectorAll('.form-control').forEach(el => el.classList.remove('error'));
        }
    }
}

function showConfirm(title, message, onConfirm) {
    const modal = document.getElementById('confirm-modal');
    document.getElementById('confirm-modal-title').textContent = title;
    document.getElementById('confirm-modal-message').textContent = message;

    const confirmBtn = document.getElementById('confirm-modal-btn');
    // Replace old event listener
    const newBtn = confirmBtn.cloneNode(true);
    confirmBtn.parentNode.replaceChild(newBtn, confirmBtn);

    newBtn.addEventListener('click', async () => {
        closeModal('confirm-modal');
        await onConfirm();
    });

    openModal('confirm-modal');
}

// ==========================================
// NAVIGATION & TABS
// ==========================================
function switchTab(tabId) {
    state.activeTab = tabId;

    document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.nav-item').forEach(el => el.classList.remove('active'));

    const targetSection = document.getElementById(`tab-${tabId}`);
    if (targetSection) targetSection.classList.add('active');

    const targetNav = document.querySelector(`.nav-item[data-tab="${tabId}"]`);
    if (targetNav) targetNav.classList.add('active');

    // Update Topbar Titles
    const titleEl = document.getElementById('current-page-title');
    const subtitleEl = document.getElementById('current-page-subtitle');

    switch (tabId) {
        case 'dashboard':
            titleEl.textContent = 'Dashboard Overview';
            subtitleEl.textContent = 'College academic and enrollment administration';
            loadDashboard();
            break;
        case 'students':
            titleEl.textContent = 'Student Directory';
            subtitleEl.textContent = 'Manage student enrollments, profiles, and records';
            loadStudents();
            break;
        case 'courses':
            titleEl.textContent = 'Degree Programs & Courses';
            subtitleEl.textContent = 'Departmental curricula and student affiliations';
            loadCourses();
            break;
        case 'results':
            titleEl.textContent = 'Results & Grade Ledger';
            subtitleEl.textContent = 'Exams performance metrics and passing records';
            loadResults();
            break;
        case 'profile':
            titleEl.textContent = 'Student Profile';
            subtitleEl.textContent = 'Detailed academic ledger and contact info';
            break;
    }
}

// ==========================================
// 1. DASHBOARD MODULE
// ==========================================
async function loadDashboard() {
    try {
        const stats = await apiRequest('/api/dashboard/stats');

        document.getElementById('stat-total-students').textContent = stats.total_students || 0;
        document.getElementById('stat-total-courses').textContent = stats.total_courses || 0;
        document.getElementById('stat-avg-marks').textContent = (stats.average_marks || 0).toFixed(1);
        document.getElementById('stat-avg-percentage').textContent = (stats.average_percentage || 0).toFixed(1) + '%';
        document.getElementById('stat-passing-count').textContent = stats.passing_count || 0;
        document.getElementById('stat-failing-count').textContent = stats.failing_count || 0;
        document.getElementById('chart-total-badge').textContent = `${stats.total_students || 0} Evaluated`;

        // Render Grade Distribution Bars
        renderGradeBars(stats.grade_distribution || {}, stats.total_students || 0);

        // Render Recent Students
        renderRecentStudents(stats.recent_students || []);
    } catch (err) {
        showToast('Failed to load dashboard data: ' + err.message, 'error');
    }
}

function renderGradeBars(dist, total) {
    const container = document.getElementById('grade-bars-container');
    const grades = [
        { label: 'A+', key: 'A+', fillClass: 'fill-aplus', range: '90-100%' },
        { label: 'A', key: 'A', fillClass: 'fill-a', range: '80-89.9%' },
        { label: 'B', key: 'B', fillClass: 'fill-b', range: '70-79.9%' },
        { label: 'C', key: 'C', fillClass: 'fill-c', range: '60-69.9%' },
        { label: 'D', key: 'D', fillClass: 'fill-d', range: '40-59.9%' },
        { label: 'F', key: 'F', fillClass: 'fill-f', range: '<40%' }
    ];

    container.innerHTML = grades.map(g => {
        const count = dist[g.key] || 0;
        const pct = total > 0 ? ((count / total) * 100).toFixed(1) : 0;
        return `
            <div class="grade-bar-item">
                <div class="grade-label">
                    <span class="badge ${getGradeBadgeClass(g.label)}">${g.label}</span>
                </div>
                <div class="grade-progress-wrap" title="${g.range}: ${count} student(s)">
                    <div class="grade-progress-fill ${g.fillClass}" style="width: ${pct}%"></div>
                </div>
                <div class="grade-count-label">${count} <small>(${pct}%)</small></div>
            </div>
        `;
    }).join('');
}

function renderRecentStudents(recentList) {
    const tbody = document.getElementById('recent-students-tbody');
    if (!recentList || recentList.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="8" class="empty-state">
                    <h3>No students enrolled yet</h3>
                    <p>Click "Add Student" to create your first record</p>
                </td>
            </tr>
        `;
        return;
    }

    tbody.innerHTML = recentList.map(s => `
        <tr>
            <td><strong>${escapeHtml(s.student_id)}</strong></td>
            <td>${escapeHtml(s.full_name)}</td>
            <td>${escapeHtml(s.course_name)}</td>
            <td><strong>${s.percentage.toFixed(2)}%</strong></td>
            <td><span class="badge ${getGradeBadgeClass(s.grade)}">${s.grade}</span></td>
            <td><span class="badge ${s.status === 'Pass' ? 'badge-pass' : 'badge-fail'}">${s.status}</span></td>
            <td>${formatDate(s.admission_date)}</td>
            <td>
                <button class="btn btn-secondary btn-sm" onclick="viewStudentProfile(${s.id})">View Profile</button>
            </td>
        </tr>
    `).join('');
}

// ==========================================
// 2. STUDENTS MODULE
// ==========================================
async function loadStudents() {
    const { search, courseId, grade, sortBy, sortDir } = state.studentFilters;
    const { page, limit } = state.studentPagination;

    const params = new URLSearchParams({
        search,
        course_id: courseId,
        grade,
        sort_by: sortBy,
        sort_dir: sortDir,
        page,
        limit
    });

    try {
        const data = await apiRequest(`/api/students?${params.toString()}`);
        state.students = data.students || [];
        state.studentPagination.total = data.total || 0;
        state.studentPagination.totalPages = data.total_pages || 1;

        renderStudentsTable();
        renderPagination();
    } catch (err) {
        showToast('Error loading students: ' + err.message, 'error');
    }
}

function renderStudentsTable() {
    const tbody = document.getElementById('students-tbody');
    if (!state.students || state.students.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="9" class="empty-state">
                    <h3>No students match your query</h3>
                    <p>Try clearing filters or adding a new student</p>
                    <button class="btn btn-primary btn-sm" onclick="openAddStudentModal()">+ Add New Student</button>
                </td>
            </tr>
        `;
        return;
    }

    tbody.innerHTML = state.students.map(s => `
        <tr>
            <td><strong>${escapeHtml(s.student_id)}</strong></td>
            <td>
                <div style="font-weight: 600;">${escapeHtml(s.full_name)}</div>
                <div style="font-size: 11px; color: var(--text-muted);">${escapeHtml(s.email)}</div>
            </td>
            <td>
                <div>${escapeHtml(s.course_name)}</div>
                <div style="font-size: 11px; color: var(--text-muted);">${escapeHtml(s.department)}</div>
            </td>
            <td>Sem ${s.semester}</td>
            <td>${s.marks_obtained.toFixed(1)} / ${s.maximum_marks.toFixed(1)}</td>
            <td><strong>${s.percentage.toFixed(2)}%</strong></td>
            <td><span class="badge ${getGradeBadgeClass(s.grade)}">${s.grade}</span></td>
            <td><span class="badge ${s.status === 'Pass' ? 'badge-pass' : 'badge-fail'}">${s.status}</span></td>
            <td>
                <div class="table-actions">
                    <button class="btn btn-secondary btn-icon-only" title="View Profile" onclick="viewStudentProfile(${s.id})">
                        <svg viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
                    </button>
                    <button class="btn btn-secondary btn-icon-only" title="Edit Student" onclick="editStudent(${s.id})">
                        <svg viewBox="0 0 24 24"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"/></svg>
                    </button>
                    <button class="btn btn-outline-danger btn-icon-only" title="Delete Student" onclick="deleteStudent(${s.id}, '${escapeHtml(s.full_name)}')">
                        <svg viewBox="0 0 24 24"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>
                    </button>
                </div>
            </td>
        </tr>
    `).join('');
}

function renderPagination() {
    const { page, totalPages, total, limit } = state.studentPagination;
    const info = document.getElementById('pagination-info');
    const controls = document.getElementById('pagination-controls');

    const start = total === 0 ? 0 : (page - 1) * limit + 1;
    const end = Math.min(page * limit, total);
    info.textContent = `Showing ${start}-${end} of ${total} students`;

    controls.innerHTML = `
        <button class="page-btn" ${page <= 1 ? 'disabled' : ''} onclick="goToStudentPage(${page - 1})">Prev</button>
        <span style="font-size: 12px; font-weight: 600; padding: 0 8px;">Page ${page} of ${totalPages}</span>
        <button class="page-btn" ${page >= totalPages ? 'disabled' : ''} onclick="goToStudentPage(${page + 1})">Next</button>
    `;
}

function goToStudentPage(newPage) {
    state.studentPagination.page = newPage;
    loadStudents();
}

// Student Modal: Add
function openAddStudentModal() {
    document.getElementById('student-modal-title').textContent = 'Add New Student';
    document.getElementById('student-db-id').value = '';
    document.getElementById('student-form').reset();
    document.getElementById('field-max-marks').value = '500';
    document.getElementById('field-semester').value = '1';
    document.getElementById('field-admission-date').value = new Date().toISOString().split('T')[0];
    populateCourseSelect('field-course');
    openModal('student-modal');
}

// Student Modal: Edit
async function editStudent(id) {
    try {
        const data = await apiRequest(`/api/students/${id}`);
        const s = data.student;

        document.getElementById('student-modal-title').textContent = 'Edit Student Details';
        document.getElementById('student-db-id').value = s.id;
        document.getElementById('field-student-id').value = s.student_id;
        document.getElementById('field-full-name').value = s.full_name;
        document.getElementById('field-email').value = s.email;
        document.getElementById('field-phone').value = s.phone || '';
        populateCourseSelect('field-course', s.course_id);
        document.getElementById('field-semester').value = s.semester;
        document.getElementById('field-marks').value = s.marks_obtained;
        document.getElementById('field-max-marks').value = s.maximum_marks;
        document.getElementById('field-admission-date').value = s.admission_date;

        openModal('student-modal');
    } catch (err) {
        showToast('Could not load student for editing: ' + err.message, 'error');
    }
}

// Student Delete
function deleteStudent(id, name) {
    showConfirm(
        'Delete Student Record',
        `Are you sure you want to permanently delete the student record for "${name}"? This action cannot be undone.`,
        async () => {
            try {
                await apiRequest(`/api/students/${id}`, 'DELETE');
                showToast(`Student "${name}" deleted successfully.`, 'success');
                loadStudents();
                if (state.activeTab === 'dashboard') loadDashboard();
                if (state.activeTab === 'profile' && state.activeStudentProfileId === id) {
                    switchTab('students');
                }
            } catch (err) {
                showToast('Failed to delete student: ' + err.message, 'error');
            }
        }
    );
}

// Student Form Save Handler
document.getElementById('student-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    const id = document.getElementById('student-db-id').value;
    const isEdit = !!id;

    const studentData = {
        student_id: document.getElementById('field-student-id').value.trim(),
        full_name: document.getElementById('field-full-name').value.trim(),
        email: document.getElementById('field-email').value.trim(),
        phone: document.getElementById('field-phone').value.trim(),
        course_id: parseInt(document.getElementById('field-course').value),
        semester: parseInt(document.getElementById('field-semester').value),
        marks_obtained: parseFloat(document.getElementById('field-marks').value),
        maximum_marks: parseFloat(document.getElementById('field-max-marks').value),
        admission_date: document.getElementById('field-admission-date').value
    };

    // Frontend validations
    if (studentData.marks_obtained < 0) {
        showToast('Marks obtained cannot be negative.', 'warning');
        return;
    }
    if (studentData.maximum_marks <= 0) {
        showToast('Maximum marks must be greater than zero.', 'warning');
        return;
    }
    if (studentData.marks_obtained > studentData.maximum_marks) {
        showToast('Marks obtained cannot exceed maximum marks.', 'warning');
        return;
    }

    try {
        if (isEdit) {
            await apiRequest(`/api/students/${id}`, 'PUT', studentData);
            showToast('Student record updated successfully!', 'success');
            if (state.activeTab === 'profile' && state.activeStudentProfileId === parseInt(id)) {
                viewStudentProfile(id);
            }
        } else {
            await apiRequest('/api/students', 'POST', studentData);
            showToast('New student enrolled successfully!', 'success');
        }

        closeModal('student-modal');
        loadStudents();
        loadDashboard();
    } catch (err) {
        showToast(err.message, 'error');
    }
});

// View Student Profile
async function viewStudentProfile(id) {
    try {
        const data = await apiRequest(`/api/students/${id}`);
        const s = data.student;
        state.activeStudentProfileId = s.id;

        const profileContainer = document.getElementById('student-profile-view');
        profileContainer.innerHTML = `
            <div class="profile-header-strip">
                <div class="profile-avatar">${escapeHtml(s.full_name.charAt(0).toUpperCase())}</div>
                <div class="profile-main-info">
                    <h2>${escapeHtml(s.full_name)}</h2>
                    <p>Roll / Student ID: <strong>${escapeHtml(s.student_id)}</strong> • Enrolled in ${escapeHtml(s.course_name)}</p>
                </div>
                <div style="margin-left: auto; display: flex; gap: 10px;">
                    <button class="btn btn-secondary btn-sm" onclick="editStudent(${s.id})">Edit Profile</button>
                    <button class="btn btn-danger btn-sm" onclick="deleteStudent(${s.id}, '${escapeHtml(s.full_name)}')">Delete</button>
                </div>
            </div>

            <div class="profile-meta-grid">
                <div class="profile-item">
                    <label>Email Address</label>
                    <span>${escapeHtml(s.email)}</span>
                </div>
                <div class="profile-item">
                    <label>Phone Number</label>
                    <span>${escapeHtml(s.phone || 'Not provided')}</span>
                </div>
                <div class="profile-item">
                    <label>Department</label>
                    <span>${escapeHtml(s.department)}</span>
                </div>
                <div class="profile-item">
                    <label>Semester</label>
                    <span>Semester ${s.semester}</span>
                </div>
                <div class="profile-item">
                    <label>Marks Scored</label>
                    <span>${s.marks_obtained.toFixed(1)} / ${s.maximum_marks.toFixed(1)}</span>
                </div>
                <div class="profile-item">
                    <label>Calculated Percentage</label>
                    <span style="font-size: 16px; color: var(--primary);">${s.percentage.toFixed(2)}%</span>
                </div>
                <div class="profile-item">
                    <label>Academic Grade</label>
                    <span><span class="badge ${getGradeBadgeClass(s.grade)}">${s.grade}</span></span>
                </div>
                <div class="profile-item">
                    <label>Result Status</label>
                    <span><span class="badge ${s.status === 'Pass' ? 'badge-pass' : 'badge-fail'}">${s.status}</span></span>
                </div>
                <div class="profile-item">
                    <label>Date of Admission</label>
                    <span>${formatDate(s.admission_date)}</span>
                </div>
                <div class="profile-item">
                    <label>Record Created</label>
                    <span>${s.created_at || 'N/A'}</span>
                </div>
            </div>
        `;

        document.getElementById('nav-profile-btn').style.display = 'flex';
        switchTab('profile');
    } catch (err) {
        showToast('Error loading profile: ' + err.message, 'error');
    }
}

// ==========================================
// 3. COURSES MODULE
// ==========================================
async function loadCourses() {
    try {
        const data = await apiRequest('/api/courses');
        state.courses = data.courses || [];
        renderCourses();
        populateAllCourseSelects();
    } catch (err) {
        showToast('Error loading courses: ' + err.message, 'error');
    }
}

function renderCourses() {
    const grid = document.getElementById('courses-grid');
    if (!state.courses || state.courses.length === 0) {
        grid.innerHTML = `
            <div class="card empty-state" style="grid-column: 1 / -1;">
                <h3>No courses configured</h3>
                <p>Add degree programs or courses to enroll students</p>
                <button class="btn btn-primary btn-sm" onclick="openAddCourseModal()">+ Add New Course</button>
            </div>
        `;
        return;
    }

    grid.innerHTML = state.courses.map(c => `
        <div class="course-card">
            <div>
                <div class="course-card-top">
                    <span class="course-dept">${escapeHtml(c.department)}</span>
                    <span class="badge badge-pass">${c.student_count || 0} Enrolled</span>
                </div>
                <h3 class="course-title">${escapeHtml(c.course_name)}</h3>
                <p class="course-desc">${escapeHtml(c.description || 'No description provided for this academic program.')}</p>
            </div>

            <div class="course-card-footer">
                <div class="course-meta">
                    <span>⏱ ${escapeHtml(c.duration)}</span>
                </div>
                <div class="table-actions">
                    <button class="btn btn-secondary btn-icon-only" title="Edit Course" onclick="editCourse(${c.id})">
                        <svg viewBox="0 0 24 24"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"/></svg>
                    </button>
                    <button class="btn btn-outline-danger btn-icon-only" title="Delete Course" onclick="deleteCourse(${c.id}, '${escapeHtml(c.course_name)}')">
                        <svg viewBox="0 0 24 24"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>
                    </button>
                </div>
            </div>
        </div>
    `).join('');
}

function populateCourseSelect(selectId, selectedId = null) {
    const sel = document.getElementById(selectId);
    if (!sel) return;

    const currentVal = selectedId || sel.value;
    sel.innerHTML = '<option value="">Select Course</option>' +
        state.courses.map(c => `<option value="${c.id}" ${c.id == currentVal ? 'selected' : ''}>${escapeHtml(c.course_name)}</option>`).join('');
}

function populateAllCourseSelects() {
    populateCourseSelect('field-course');

    // Populates filter dropdowns
    const stFilter = document.getElementById('students-filter-course');
    if (stFilter) {
        stFilter.innerHTML = '<option value="0">All Courses</option>' +
            state.courses.map(c => `<option value="${c.id}">${escapeHtml(c.course_name)}</option>`).join('');
    }

    const resFilter = document.getElementById('results-filter-course');
    if (resFilter) {
        resFilter.innerHTML = '<option value="0">All Courses</option>' +
            state.courses.map(c => `<option value="${c.id}">${escapeHtml(c.course_name)}</option>`).join('');
    }
}

function openAddCourseModal() {
    document.getElementById('course-modal-title').textContent = 'Add New Course';
    document.getElementById('course-db-id').value = '';
    document.getElementById('course-form').reset();
    openModal('course-modal');
}

async function editCourse(id) {
    try {
        const data = await apiRequest(`/api/courses/${id}`);
        const c = data.course;

        document.getElementById('course-modal-title').textContent = 'Edit Course Details';
        document.getElementById('course-db-id').value = c.id;
        document.getElementById('field-course-name').value = c.course_name;
        document.getElementById('field-department').value = c.department;
        document.getElementById('field-duration').value = c.duration;
        document.getElementById('field-description').value = c.description || '';

        openModal('course-modal');
    } catch (err) {
        showToast('Error loading course: ' + err.message, 'error');
    }
}

function deleteCourse(id, name) {
    showConfirm(
        'Delete Course Program',
        `Are you sure you want to delete the course "${name}"? If students are enrolled, deletion will be blocked to protect record integrity.`,
        async () => {
            try {
                await apiRequest(`/api/courses/${id}`, 'DELETE');
                showToast(`Course "${name}" deleted successfully.`, 'success');
                loadCourses();
                loadDashboard();
            } catch (err) {
                showToast(err.message, 'error');
            }
        }
    );
}

document.getElementById('course-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    const id = document.getElementById('course-db-id').value;
    const isEdit = !!id;

    const courseData = {
        course_name: document.getElementById('field-course-name').value.trim(),
        department: document.getElementById('field-department').value.trim(),
        duration: document.getElementById('field-duration').value.trim(),
        description: document.getElementById('field-description').value.trim()
    };

    try {
        if (isEdit) {
            await apiRequest(`/api/courses/${id}`, 'PUT', courseData);
            showToast('Course updated successfully!', 'success');
        } else {
            await apiRequest('/api/courses', 'POST', courseData);
            showToast('New course created successfully!', 'success');
        }

        closeModal('course-modal');
        loadCourses();
        loadDashboard();
    } catch (err) {
        showToast(err.message, 'error');
    }
});

// ==========================================
// 4. RESULTS MODULE
// ==========================================
async function loadResults() {
    const { search, courseId, grade, status } = state.resultsFilters;
    const params = new URLSearchParams({
        search,
        course_id: courseId,
        grade,
        status
    });

    try {
        const data = await apiRequest(`/api/results?${params.toString()}`);
        const summary = data.summary || {};
        const records = data.records || [];

        document.getElementById('res-total').textContent = summary.total || 0;
        document.getElementById('res-passed').textContent = summary.passed || 0;
        document.getElementById('res-failed').textContent = summary.failed || 0;
        document.getElementById('res-pass-rate').textContent = (summary.pass_rate || 0).toFixed(1) + '%';
        document.getElementById('res-avg-score').textContent = (summary.average_percentage || 0).toFixed(1) + '%';

        renderResultsTable(records);
    } catch (err) {
        showToast('Error loading results: ' + err.message, 'error');
    }
}

function renderResultsTable(records) {
    const tbody = document.getElementById('results-tbody');
    if (!records || records.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="10" class="empty-state">
                    <h3>No result records match criteria</h3>
                    <p>Adjust your search filters or add marks to students</p>
                </td>
            </tr>
        `;
        return;
    }

    tbody.innerHTML = records.map((r, idx) => `
        <tr>
            <td><strong>#${idx + 1}</strong></td>
            <td><strong>${escapeHtml(r.student_id)}</strong></td>
            <td>${escapeHtml(r.full_name)}</td>
            <td>${escapeHtml(r.course_name)}</td>
            <td>Sem ${r.semester}</td>
            <td>${r.marks_obtained.toFixed(1)}</td>
            <td>${r.maximum_marks.toFixed(1)}</td>
            <td><strong style="color: var(--primary);">${r.percentage.toFixed(2)}%</strong></td>
            <td><span class="badge ${getGradeBadgeClass(r.grade)}">${r.grade}</span></td>
            <td><span class="badge ${r.status === 'Pass' ? 'badge-pass' : 'badge-fail'}">${r.status}</span></td>
        </tr>
    `).join('');
}

// ==========================================
// EVENT LISTENERS & FILTER WIRING
// ==========================================
async function checkHealth() {
    const statusText = document.getElementById('backend-status-text');
    const statusDot = document.querySelector('.status-dot');
    try {
        const health = await apiRequest('/api/health');
        if (statusText) statusText.textContent = 'C++ Backend Online';
        if (statusDot) {
            statusDot.style.backgroundColor = '#34d399';
            statusDot.style.boxShadow = '0 0 8px #34d399';
        }
    } catch {
        if (statusText) statusText.textContent = 'Backend Offline (Click to configure)';
        if (statusDot) {
            statusDot.style.backgroundColor = '#ef4444';
            statusDot.style.boxShadow = '0 0 8px #ef4444';
        }
    }
}

function openApiSettingsModal() {
    const current = resolveApiBase();
    const input = document.getElementById('field-api-url');
    const code = document.getElementById('active-api-code');
    if (input) input.value = current;
    if (code) code.textContent = current ? current : '(Local relative /api/...)';
    openModal('api-settings-modal');
}

document.addEventListener('DOMContentLoaded', async () => {
    // Health check on load
    await checkHealth();

    // API settings modal handlers
    const btnApiSettings = document.getElementById('btn-api-settings');
    if (btnApiSettings) {
        btnApiSettings.addEventListener('click', (e) => {
            e.stopPropagation();
            openApiSettingsModal();
        });
    }

    const backendStatusBadge = document.getElementById('backend-status-badge');
    if (backendStatusBadge) {
        backendStatusBadge.addEventListener('click', openApiSettingsModal);
    }

    const btnSaveApi = document.getElementById('btn-save-api');
    if (btnSaveApi) {
        btnSaveApi.addEventListener('click', async () => {
            const input = document.getElementById('field-api-url');
            const val = input ? input.value.trim().replace(/\/+$/, '') : '';
            if (val) {
                localStorage.setItem('EDUTRACK_API_BASE', val);
            } else {
                localStorage.removeItem('EDUTRACK_API_BASE');
            }
            API_BASE = resolveApiBase();
            closeModal('api-settings-modal');
            showToast('Backend API URL updated. Reconnecting...', 'info');
            await checkHealth();
            await loadCourses();
            await loadDashboard();
            if (state.activeTab === 'students') await loadStudents();
            if (state.activeTab === 'results') await loadResults();
        });
    }

    const btnResetApi = document.getElementById('btn-reset-api');
    if (btnResetApi) {
        btnResetApi.addEventListener('click', async () => {
            localStorage.removeItem('EDUTRACK_API_BASE');
            API_BASE = resolveApiBase();
            const input = document.getElementById('field-api-url');
            if (input) input.value = '';
            closeModal('api-settings-modal');
            showToast('Reset to local relative API endpoint.', 'info');
            await checkHealth();
            await loadCourses();
            await loadDashboard();
            if (state.activeTab === 'students') await loadStudents();
            if (state.activeTab === 'results') await loadResults();
        });
    }

    // Load initial courses and dashboard
    await loadCourses();
    await loadDashboard();

    // Tab buttons
    document.querySelectorAll('.nav-item').forEach(btn => {
        btn.addEventListener('click', () => {
            const tab = btn.getAttribute('data-tab');
            if (tab) switchTab(tab);
        });
    });

    // Mobile sidebar toggle
    const menuToggle = document.getElementById('menuToggle');
    const sidebar = document.getElementById('sidebar');
    if (menuToggle && sidebar) {
        menuToggle.addEventListener('click', () => {
            sidebar.classList.toggle('mobile-open');
        });
    }

    // Modal close buttons (data-close-modal)
    document.querySelectorAll('[data-close-modal]').forEach(btn => {
        btn.addEventListener('click', () => {
            const modalId = btn.getAttribute('data-close-modal');
            closeModal(modalId);
        });
    });

    // Close modal on click outside box
    document.querySelectorAll('.modal-overlay').forEach(modal => {
        modal.addEventListener('click', (e) => {
            if (e.target === modal) closeModal(modal.id);
        });
    });

    // Topbar Add Student button
    document.getElementById('topbar-add-student-btn').addEventListener('click', openAddStudentModal);
    document.getElementById('btn-open-add-student').addEventListener('click', openAddStudentModal);
    document.getElementById('btn-open-add-course').addEventListener('click', openAddCourseModal);
    document.getElementById('qa-add-student').addEventListener('click', openAddStudentModal);
    document.getElementById('qa-add-course').addEventListener('click', openAddCourseModal);
    document.getElementById('qa-view-results').addEventListener('click', () => switchTab('results'));
    document.getElementById('btn-view-all-students').addEventListener('click', () => switchTab('students'));
    document.getElementById('btn-back-to-students').addEventListener('click', () => switchTab('students'));

    // Student Filter Controls
    const stSearch = document.getElementById('students-search');
    let debounceTimer;
    stSearch.addEventListener('input', () => {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            state.studentFilters.search = stSearch.value.trim();
            state.studentPagination.page = 1;
            loadStudents();
        }, 300);
    });

    document.getElementById('students-filter-course').addEventListener('change', (e) => {
        state.studentFilters.courseId = parseInt(e.target.value) || 0;
        state.studentPagination.page = 1;
        loadStudents();
    });

    document.getElementById('students-filter-grade').addEventListener('change', (e) => {
        state.studentFilters.grade = e.target.value;
        state.studentPagination.page = 1;
        loadStudents();
    });

    document.getElementById('students-sort').addEventListener('change', (e) => {
        const val = e.target.value;
        const [sortBy, sortDir] = val.split('_');
        state.studentFilters.sortBy = sortBy;
        state.studentFilters.sortDir = sortDir;
        loadStudents();
    });

    // Results Filter Controls
    const resSearch = document.getElementById('results-search');
    resSearch.addEventListener('input', () => {
        clearTimeout(debounceTimer);
        debounceTimer = setTimeout(() => {
            state.resultsFilters.search = resSearch.value.trim();
            loadResults();
        }, 300);
    });

    document.getElementById('results-filter-course').addEventListener('change', (e) => {
        state.resultsFilters.courseId = parseInt(e.target.value) || 0;
        loadResults();
    });

    document.getElementById('results-filter-grade').addEventListener('change', (e) => {
        state.resultsFilters.grade = e.target.value;
        loadResults();
    });

    document.getElementById('results-filter-status').addEventListener('change', (e) => {
        state.resultsFilters.status = e.target.value;
        loadResults();
    });

    // Table Header Sorting
    document.querySelectorAll('#students-table th.sortable').forEach(th => {
        th.addEventListener('click', () => {
            const sortField = th.getAttribute('data-sort');
            if (state.studentFilters.sortBy === sortField) {
                state.studentFilters.sortDir = state.studentFilters.sortDir === 'asc' ? 'desc' : 'asc';
            } else {
                state.studentFilters.sortBy = sortField;
                state.studentFilters.sortDir = 'asc';
            }
            loadStudents();
        });
    });
});
