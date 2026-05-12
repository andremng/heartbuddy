namespace HeartBuddyAPI.Models
{
    public class Threshold
    {
        public int ThresholdID { get; set; }
        public string Parameter { get; set; } = string.Empty;
        public float MinValue { get; set; }
        public float MaxValue { get; set; }
    }
}
