﻿using Nefarius.ViGEm.Client;
using System;
using Nefarius.ViGEm.Client.Targets;
using Nefarius.ViGEm.Client.Targets.Xbox360;

namespace ViGEmTester.NET
{
    internal class Program
    {
        private static void Main(string[] args)
        {
            var client = new ViGEmClient();

            var x360 = new Xbox360Controller(client);

            x360.FeedbackReceived +=
                (sender, eventArgs) => Console.WriteLine(
                    $"LM: {eventArgs.LargeMotor}, " +
                    $"SM: {eventArgs.SmallMotor}, " +
                    $"LED: {eventArgs.LedNumber}");

            x360.Connect();

            var report = new Xbox360Report();
            report.SetButtons(Xbox360Buttons.A, Xbox360Buttons.B);
            report.SetAxis(Xbox360Axes.LeftTrigger, 0xFF);
            report.SetAxis(Xbox360Axes.RightTrigger, 0xFF);

            x360.SendReport(report);

            /*
            var x360 = new Xbox360Controller();
            x360.FeedbackReceived +=
                (sender, eventArgs) => Console.WriteLine(
                    $"LM: {eventArgs.LargeMotor}, " +
                    $"SM: {eventArgs.SmallMotor}, " +
                    $"LED: {eventArgs.LedNumber}");
            x360.PlugIn();

            var report = new Xbox360Report();
            report.SetButtons(Xbox360Buttons.A, Xbox360Buttons.B);
            report.SetAxis(Xbox360Axes.LeftTrigger, 0xFF);
            report.SetAxis(Xbox360Axes.RightTrigger, 0xFF);

            x360.SendReport(report);

            Console.ReadKey();

            var ds4 = new DualShock4Controller();
            ds4.FeedbackReceived +=
                (sender, eventArgs) => Console.WriteLine(
                    $"LM: {eventArgs.LargeMotor}, " +
                    $"SM: {eventArgs.SmallMotor}, ");
            ds4.PlugIn();
            */

            Console.ReadKey();
        }
    }
}