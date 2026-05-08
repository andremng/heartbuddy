using Microsoft.EntityFrameworkCore;
using HeartBuddyAPI.Models;

namespace HeartBuddyAPI
{
    public class AppDbContext : DbContext
    {
        public AppDbContext(DbContextOptions<AppDbContext> options) : base(options) { }

        public DbSet<Reading> Readings { get; set; }
        public DbSet<Device> Devices { get; set; }
        public DbSet<User> Users { get; set; }
        public DbSet<Alert> Alerts { get; set; }
        public DbSet<Threshold> Thresholds { get; set; }
    }
}
