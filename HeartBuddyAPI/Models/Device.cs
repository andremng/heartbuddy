namespace HeartBuddyAPI.Models
{
    public class Device
    {
        public int DeviceID { get; set; }
        public string DeviceName { get; set; } = string.Empty;
        public DateTime RegistrationDate { get; set; }
        public int UserID { get; set; }
    }
}
