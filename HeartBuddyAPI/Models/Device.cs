namespace HeartBuddyAPI.Models
{
    public class Device
    {
        public int DeviceID { get; set; }
        public string DeviceName { get; set; } = string.Empty;
        public string Location { get; set; } = string.Empty;
        public DateTime CreatedAt { get; set; }
    }
}
