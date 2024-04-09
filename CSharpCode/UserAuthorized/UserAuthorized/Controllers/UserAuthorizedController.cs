using Microsoft.AspNetCore.Mvc;

//use this to retrieve request https://localhost:7080/UserAuthorized?FingerID=1&EquipID=1
//from Azure  https://testserviceapi2.azure-api.net/UserAuthorized/UserAuthorized?FingerID=1&EquipID=2



namespace UserAuthorized.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class UserAuthorizedController : ControllerBase
    {
        private readonly ILogger<UserAuthorizedController> _logger;

        public UserAuthorizedController(ILogger<UserAuthorizedController> logger)
        {
            _logger = logger;
        }

        [HttpGet(Name = "IsUserAuthorized")]
        public string Get(string FingerID, string EquipID)
        {
            string theUser = Functions.GetUsername(FingerID, EquipID);

            return theUser;
        }
    }
}
