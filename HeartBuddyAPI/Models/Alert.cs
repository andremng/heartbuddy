namespace HeartBuddyAPI.Models
{
    public class Alert
    {
        public int AlertID { get; set; }
        public int ReadingID { get; set; }
        public string AlertType { get; set; } = string.Empty;
        public string Severity { get; set; } = string.Empty;
        public bool Acknowledged { get; set; }
        public DateTime CreatedAt { get; set; }
    }
}
