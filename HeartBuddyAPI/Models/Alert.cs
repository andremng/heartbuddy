namespace HeartBuddyAPI.Models
{
    public class Alert
    {
        public int AlertID { get; set; }
        public int DeviceID { get; set; }
        public string Message { get; set; } = string.Empty;
        public string Severity { get; set; } = string.Empty;
        public DateTime CreatedAt { get; set; }
    }
}
