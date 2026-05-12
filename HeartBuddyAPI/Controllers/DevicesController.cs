using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using HeartBuddyAPI.Models;

namespace HeartBuddyAPI.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class DevicesController : ControllerBase
    {
        private readonly AppDbContext _context;

        public DevicesController(AppDbContext context)
        {
            _context = context;
        }

        // GET - all devices
        [HttpGet]
        public async Task<IActionResult> GetAllDevices()
        {
            var devices = await _context.Devices.ToListAsync();
            return Ok(devices);
        }
    }
}
