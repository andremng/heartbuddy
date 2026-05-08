namespace HeartBuddyAPI.Models
{
    public class Reading
    {
        public int ReadingID { get; set; }
        public int DeviceID { get; set; }
        public int HeartRate { get; set; }
        public float Sp02 { get; set; }
        public int Movement { get; set; }
        public DateTime Timestamp { get; set; }
    }
}
