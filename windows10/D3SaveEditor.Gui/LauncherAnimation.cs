// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace D3SaveEditor.Gui
{
    internal sealed class LauncherAnimation
    {
        private const int FrameCount = 9;
        private static readonly TimeSpan FrameInterval = TimeSpan.FromMilliseconds(100);

        private readonly Image image;
        private readonly BitmapFrame[] frames;
        private readonly DispatcherTimer timer;
        private int currentFrame;

        private LauncherAnimation(Image image, BitmapFrame[] frames)
        {
            this.image = image;
            this.frames = frames;
            ShowFrame();
            image.Visibility = Visibility.Visible;

            timer = new DispatcherTimer(DispatcherPriority.Background, image.Dispatcher)
            {
                Interval = FrameInterval
            };
            timer.Tick += AdvanceFrame;
            timer.Start();
        }

        internal static LauncherAnimation TryStart(Image image, string directory)
        {
            var frames = new BitmapFrame[FrameCount];
            try
            {
                for (int index = 0; index < frames.Length; index++)
                {
                    using (var input = File.OpenRead(Path.Combine(directory, $"Anim{index}.bmp")))
                    {
                        var decoder = new BmpBitmapDecoder(input, BitmapCreateOptions.PreservePixelFormat,
                            BitmapCacheOption.OnLoad);
                        frames[index] = decoder.Frames[0];
                        frames[index].Freeze();
                    }
                }
            }
            catch (Exception failure) when (failure is IOException || failure is UnauthorizedAccessException ||
                failure is FileFormatException || failure is NotSupportedException)
            {
                return null;
            }

            return new LauncherAnimation(image, frames);
        }

        internal void Stop()
        {
            timer.Stop();
            timer.Tick -= AdvanceFrame;
        }

        private void AdvanceFrame(object sender, EventArgs e)
        {
            currentFrame = (currentFrame + 1) % frames.Length;
            ShowFrame();
        }

        private void ShowFrame()
        {
            var frame = frames[currentFrame];
            var dpi = VisualTreeHelper.GetDpi(image);
            // Use the bitmap's pixel dimensions independently of its DPI metadata.
            image.Width = frame.PixelWidth / dpi.DpiScaleX;
            image.Height = frame.PixelHeight / dpi.DpiScaleY;
            image.Source = frame;
        }
    }
}
