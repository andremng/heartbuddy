namespace HeartBuddyAPI.Models
{
    public class Reading
    {
        public int ReadingID { get; set; }
        public int DeviceID { get; set; }
        public double HeartRate { get; set; }
        public DateTime Timestamp { get; set; }
        public double? SpO2 { get; set; }
        public int? Movement { get; set; }
    }
}
