/**
 * EduTrack - Frontend API Configuration
 * 
 * LOCAL DEVELOPMENT (Default):
 * - Leave API_BASE as empty string ("") to make relative API requests (/api/...)
 *   directly to the C++ Crow web server running on localhost:18080.
 * 
 * PRODUCTION DEPLOYMENT (Vercel Frontend + Render C++ Backend):
 * - Option A (Static config): Set API_BASE to your Render backend service URL:
 *     API_BASE: "https://your-service-name.onrender.com"
 * 
 * - Option B (Runtime override): Store the backend URL in localStorage:
 *     localStorage.setItem("EDUTRACK_API_BASE", "https://your-service-name.onrender.com");
 * 
 * - Option C (UI Modal): Click the "Backend URL" button in the sidebar or topbar.
 */
window.EDUTRACK_CONFIG = {
    API_BASE: ""
};
