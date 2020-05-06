using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Bot
{
    public static class MouseButtonEventArgsExtensions
    {
        static public Point GetPoint(this MouseEventArgs obj, FrameworkElement element)
        {
            var p = obj.GetPosition(element);
            return Mouse.GetPosition(null);
        }
    }
    public partial class SelectionWindow : Window
    {
        #region Private Fields

        private bool _isMouseDown;
        private Point _startPoint;
        private Point _endPoint;

        #endregion

        #region Public Properties

        public Rect Rectangle
        {
            get
            {
                Rect rect = new Rect(_startPoint, _endPoint);
                return new Rect(rect.X / Canvas.ActualWidth, rect.Y / Canvas.ActualHeight, rect.Width / Canvas.ActualWidth, rect.Height / Canvas.ActualHeight);
            }
        }

        #endregion

        #region Life Cycle

        public SelectionWindow()
        {
            InitializeComponent();
        }

        #endregion

        #region Callbacks

        private void Canvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            _isMouseDown = true;
            _startPoint = e.GetPosition(Canvas);

            Canvas.SetLeft(Area, _startPoint.X);
            Canvas.SetTop(Area, _startPoint.Y);
        }

        private void Canvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            _endPoint = e.GetPosition(Canvas);
            _isMouseDown = false;
            DialogResult = true;
            Close();
        }

        private void Canvas_MouseMove(object sender, MouseEventArgs e)
        {
            Point currentPosition = e.GetPosition(Canvas);
            TT.Text = currentPosition.ToString();
            if (_isMouseDown)
            {
                double width = currentPosition.X - _startPoint.X;
                double height = currentPosition.Y - _startPoint.Y;
                if (width < 0)
                {
                    Canvas.SetRight(Area, _startPoint.X);
                }
                if (Height < 0)
                {
                    Canvas.SetBottom(Area, _startPoint.Y);
                }
                Area.Width = Math.Abs(width);
                Area.Height = Math.Abs(height);
            }
        }

        #endregion

        private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                Close();
            }
        }
    }
}
