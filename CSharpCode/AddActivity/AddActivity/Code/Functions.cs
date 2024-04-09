using AddActivity;
using System.Data.SqlClient;

namespace AddActivity
{

    public class Functions
    {
        private static string ConnectConfig()
        {
            return "Server=tcp:barbourserver.database.windows.net,1433;" +
            "Initial Catalog=ElectricalUsage;Persist Security Info=False;User ID=dbarbour;" +
            "Password=Pacific#123;MultipleActiveResultSets=False;Encrypt=True;" +
            "TrustServerCertificate=True;Connection Timeout=30;";
        }

        public static string AddSomeActivity(string FingerID, string EquipID, float STDCurrent, string recordType)
        {
            string theTime = DateTime.Now.ToString("MM/dd/yy HH:mm:ss");
            string theSQL = string.Format("INSERT INTO ACTIVITY(fprintposition,equipid,thedate,cumcurrent,recordtype) " +
                "VALUES ({0},{1},'{2}',{3},'{4}')", FingerID, EquipID, theTime, STDCurrent, recordType);
            using (SqlConnection pOdbcData = new SqlConnection(ConnectConfig()))
            {
                long RA = SQLHelper.ExecuteNonQuery(pOdbcData, theSQL, null);
                if (RA > 0) { return GetUsername(FingerID, EquipID); }
                else { return "0"; }
            }

        }
        public static string GetUsername(string FingerID, string EquipID)
        {
            string retMsg = "";
            string theSQL = "";
            using (SqlConnection pOdbcData = new SqlConnection(ConnectConfig()))
            {
                theSQL = string.Format("SELECT CONCAT(firstname,' ',lastname) AS CustName FROM Users us INNER JOIN Authorizations az ON az.userid = us.userid " +
                    "WHERE us.enabled = 1 AND az.fprintposition = {0} AND az.equipid = {1}", FingerID, EquipID);
                using (SqlDataReader dr = SQLHelper.ExecuteReader(pOdbcData, System.Data.CommandType.Text, theSQL, null))
                {
                    if (dr.HasRows == true)
                    {
                        dr.Read();
                        if (dr["CustName"] is null)
                        {
                            retMsg = "Unauthorized";
                        }
                        else
                        {
                            retMsg = dr["CustName"].ToString().Trim();
                        }
                    }
                    else
                    {
                        retMsg = "Unauthorized";
                    }


                }
            }
            return retMsg;
        }
    }
}

