using Microsoft.AspNetCore.Mvc;

//launch this request  https://localhost:7258/AddActivity?FingerID=1&EquipID=1&STDCurrent=19.3&recordType=InProcess
//Azure  https://testserviceapi2.azure-api.net/AddActivity/AddActivity?FingerID=1&EquipID=1&STDCurrent=38.6&recordType=Logout


namespace AddActivity.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class AddActivity : ControllerBase
    {
       

        private readonly ILogger<AddActivity> _logger;

        public AddActivity(ILogger<AddActivity> logger)
        {
            _logger = logger;
        }

        [HttpGet(Name = "AddActivity")]
        public string Get(string FingerID, string EquipID, float STDCurrent, string recordType)
        {
            string retVal = Functions.AddSomeActivity(FingerID, EquipID, STDCurrent, recordType);
            return retVal;
        }
    }
}
