// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.IO;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace D3SaveEditor.Gui
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            DispatcherUnhandledException += ShowUnhandledException;

            var view = new MainView();
            MainWindow = view;
            view.Show();

            if (e.Args.Length == 0)
                return;

            if (e.Args[0] != "--capture")
            {
                view.OpenPath(e.Args[0]);
                return;
            }

            // Developer-only screenshot route for synthetic fixtures.
            if (e.Args.Length < 3 || !view.OpenPath(e.Args[1]))
                return;

            int tab;
            if (e.Args.Length > 3 && int.TryParse(e.Args[3], out tab))
                view.ShowTab(tab);
            view.Dispatcher.InvokeAsync(() => Capture(view, e.Args[2]), DispatcherPriority.ContextIdle);
        }

        private static void Capture(MainView view, string path)
        {
            view.UpdateLayout();
            var dpi = VisualTreeHelper.GetDpi(view);
            var image = new RenderTargetBitmap(
                (int)Math.Ceiling(view.ActualWidth * dpi.DpiScaleX),
                (int)Math.Ceiling(view.ActualHeight * dpi.DpiScaleY),
                dpi.PixelsPerInchX, dpi.PixelsPerInchY, PixelFormats.Pbgra32);
            image.Render(view);

            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(image));
            using (var output = File.Create(path))
                encoder.Save(output);
            view.Close();
        }

        private void ShowUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
        {
            MessageBox.Show(e.Exception.Message, "Descent 3 Savegame Editor", MessageBoxButton.OK, MessageBoxImage.Error);
            e.Handled = true;
            if (MainWindow == null)
                Shutdown(1);
        }
    }
}
