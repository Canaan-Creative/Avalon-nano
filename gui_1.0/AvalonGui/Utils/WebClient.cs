using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;

namespace AvalonGui.Utils
{
    public class WebClient
    {
        public static String GetResponseString(string url, Encoding encoding)
        {
            HttpWebRequest request = WebRequest.Create(url) as HttpWebRequest;
            request.Method = "GET";
            request.KeepAlive = false;
            request.ContentType = "application/x-www-form-urlencoded;charset=utf8";

            return GetResponseContent(GetResponse(request), encoding);
        }

        public static HttpWebResponse GetResponse(HttpWebRequest webRequest)
        {
            HttpWebResponse response = null;

            try
            {
                response = webRequest.GetResponse() as HttpWebResponse;
            }
            catch (WebException ex)
            {
                response = ex.Response as HttpWebResponse;

                if (response == null)
                {
                    throw;
                }
            }

            return response;
        }

        public static string GetResponseContent(HttpWebResponse webResponse, Encoding encoding)
        {
            StreamReader responseReader = null;
            Stream responseStream = null;
            string responseData = null;

            try
            {
                responseStream = webResponse.GetResponseStream();
                responseReader = new StreamReader(responseStream, encoding);
                responseData = responseReader.ReadToEnd();
            }
            finally
            {
                if (responseStream != null)
                {
                    try
                    {
                        responseStream.Close();
                    }
                    catch { }
                }

                if (responseReader != null)
                {
                    try
                    {
                        responseReader.Close();
                    }
                    catch { }
                }
            }

            return responseData;
        }
    }
}
