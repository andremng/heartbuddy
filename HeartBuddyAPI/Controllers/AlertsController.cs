using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using HeartBuddyAPI.Models;

namespace HeartBuddyAPI.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AlertsController : ControllerBase
    {
        private readonly AppDbContext _context;

        public AlertsController(AppDbContext context)
        {
            _context = context;
        }

        // Get - all alerts
        [HttpGet]
        public async Task<IActionResult> GetAllAlerts()
        {
            var alerts = await _context.Alerts.ToListAsync();
            return Ok(alerts);
        } 
    }
}
