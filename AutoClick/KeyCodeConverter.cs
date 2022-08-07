using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Input;

namespace AutoClick
{
    public class KeyCodeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is ushort keyCode)
            {
                return KeyInterop.KeyFromVirtualKey(keyCode).ToString();
            }

            throw new NotImplementedException();
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
