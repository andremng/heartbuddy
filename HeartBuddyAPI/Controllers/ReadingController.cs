using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using HeartBuddyAPI.Models;

namespace HeartBuddyAPI.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class ReadingsController : ControllerBase
    {
        private readonly AppDbContext _context;

        public ReadingsController(AppDbContext context)
        {
            _context = context;
        }

        // Post - ESP32 sends data
        [HttpPost]
        public async Task<IActionResult> PostReading([FromBody] Reading reading)
        {
            reading.Timestamp = DateTime.Now;
            _context.Readings.Add(reading);
            await _context.SaveChangesAsync();
            return Ok(reading);
        }
        // Get - all readings
        [HttpGet]
        public async Task<IActionResult> GetAllReadings()
        {
            var readings = await _context.Readings.ToListAsync();
            return Ok(readings);
        }

        // Get - the latest reading
        [HttpGet("latest")]
        public async Task<IActionResult> GetLatestReading()
        {
            var latest = await _context.Readings
                .OrderByDescending(r => r.Timestamp)
                .FirstOrDefaultAsync();
                return Ok(latest);
        }

        // Get - readings from device
        [HttpGet("device/{id}")]
        public async Task<IActionResult> GetReadingsByDevice(int id)
        {
            var readings = await _context.Readings
                .Where(r => r.DeviceID == id)
                .ToListAsync();
                return Ok(readings);
        }
    }
}