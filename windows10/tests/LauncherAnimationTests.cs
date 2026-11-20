// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using D3SaveEditor.Gui;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace D3SaveEditor.Tests
{
    [TestClass]
    [DoNotParallelize]
    public sealed class LauncherAnimationTests
    {
        [STATestMethod]
        [TestCategory("WPF")]
        public void LauncherAnimationRequiresAllFramesAndPlaysThemFromMemory()
        {
            string folder = Path.Combine(Path.GetTempPath(), "D3SaveEditor.Animation-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(folder);
            var image = new Image { Visibility = Visibility.Collapsed };
            LauncherAnimation animation = null;
            try
            {
                Assert.IsNull(LauncherAnimation.TryStart(image, folder));
                for (int index = 0; index < 8; index++)
                    WriteFrame(folder, index);
                Assert.IsNull(LauncherAnimation.TryStart(image, folder));
                File.WriteAllText(Path.Combine(folder, "Anim8.bmp"), "Invalid bitmap");
                Assert.IsNull(LauncherAnimation.TryStart(image, folder));
                Assert.AreEqual(Visibility.Collapsed, image.Visibility);
                Assert.IsNull(image.Source);

                WriteFrame(folder, 8);
                animation = LauncherAnimation.TryStart(image, folder);
                Assert.IsNotNull(animation);
                Assert.AreEqual(Visibility.Visible, image.Visibility);
                var dpi = VisualTreeHelper.GetDpi(image);
                Assert.AreEqual(64.0, image.Width * dpi.DpiScaleX, 0.001);
                Assert.AreEqual(64.0, image.Height * dpi.DpiScaleY, 0.001);

                Directory.Delete(folder, true);
                CollectionAssert.AreEqual(new[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0 }, ReadFirstLoop(image));
            }
            finally
            {
                animation?.Stop();
                if (Directory.Exists(folder))
                    Directory.Delete(folder, true);
            }
        }

        private static void WriteFrame(string folder, int index)
        {
            var pixels = new byte[64 * 64];
            pixels[0] = (byte)index;
            var frame = BitmapSource.Create(64, 64, 72, 72, PixelFormats.Indexed8, BitmapPalettes.Gray256, pixels, 64);
            var encoder = new BmpBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(frame));
            using (var output = File.Create(Path.Combine(folder, $"Anim{index}.bmp")))
                encoder.Save(output);
        }

        private static List<int> ReadFirstLoop(Image image)
        {
            var shown = new List<int> { ReadFrameNumber(image) };
            var wait = new DispatcherFrame();
            var sourceProperty = DependencyPropertyDescriptor.FromProperty(Image.SourceProperty, typeof(Image));
            EventHandler record = (sender, e) =>
            {
                shown.Add(ReadFrameNumber(image));
                if (shown.Count == 10)
                    wait.Continue = false;
            };
            var timeout = new DispatcherTimer { Interval = TimeSpan.FromSeconds(5) };
            timeout.Tick += (sender, e) => wait.Continue = false;
            sourceProperty.AddValueChanged(image, record);
            timeout.Start();
            try
            {
                Dispatcher.PushFrame(wait);
                return shown;
            }
            finally
            {
                timeout.Stop();
                sourceProperty.RemoveValueChanged(image, record);
            }
        }

        private static int ReadFrameNumber(Image image)
        {
            var source = new FormatConvertedBitmap((BitmapSource)image.Source, PixelFormats.Bgr24, null, 0);
            var pixel = new byte[3];
            source.CopyPixels(new Int32Rect(0, 0, 1, 1), pixel, 3, 0);
            return pixel[0];
        }
    }
}
