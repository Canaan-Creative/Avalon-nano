using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Runtime.Serialization;

namespace AvalonGui.Model
{
    [Serializable]
    public class ModelBase
    {
        // STATUS=S,When=1407906228,Code=11,Msg=Summary,Description=bfgminer 4.4.0
        public virtual void Deserialize(string content)
        {
            PropertyInfo[] properties = this.GetType().GetProperties();

            string[] items = content.Split(",".ToCharArray());

            foreach (string item in items)
            {
                string[] keyValue = item.Split("=".ToCharArray());
                if (keyValue == null || keyValue.Length != 2) continue;

                string realKey = keyValue[0].Replace(" ", "_");

                PropertyInfo property = this.GetType().GetProperty(realKey);
                if (property != null)
                {
                    if (property.PropertyType.Name == "String")
                    {
                        property.SetValue(this, keyValue[1], null);
                    }
                    else if (property.PropertyType.Name == "Int64")
                    {
                        Int64 v = 0;
                        if (Int64.TryParse(keyValue[1], out v))
                        {
                            property.SetValue(this, v, null);
                        }
                    }
                    else if (property.PropertyType.Name == "Int32")
                    {
                        Int32 v = 0;
                        if (Int32.TryParse(keyValue[1], out v))
                        {
                            property.SetValue(this, v, null);
                        }
                    }
                    else if (property.PropertyType.Name == "Double")
                    {
                        Double v = 0;
                        if (Double.TryParse(keyValue[1], out v))
                        {
                            property.SetValue(this, v, null);
                        }
                    }
                }
            }
        }
    }
}
