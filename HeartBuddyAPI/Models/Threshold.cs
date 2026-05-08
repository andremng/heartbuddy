namespace HeartBuddyAPI.Models
{
    public class Threshold
    {
        public int ThresholdID { get; set; }
        public int DeviceID { get; set; }
        public int MinHeartRate { get; set; }
        public int MaxHeartRate { get; set; }
        public float MinSpO2 { get; set; }
        public float MaxSpO2 { get; set; }
    }
}
