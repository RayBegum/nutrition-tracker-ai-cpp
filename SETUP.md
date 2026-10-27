# Setup: run Version 1 on Windows

## Part 1 - Install Qt (one time)
Follow `WINDOWS_SETUP.md` from the earlier version, or:
1. Download the installer: https://www.qt.io/download-qt-installer-oss
2. Sign in with a free Qt account.
3. Choose Custom installation. Under the newest Qt 6.x tick **MinGW 64-bit**.
4. Under Developer and Designer Tools keep **Qt Creator**, **CMake** and **Ninja**.

## Part 2 - Get a Claude API key
1. Go to https://console.anthropic.com and sign in or create an account.
2. Open **API keys** and click **Create key**. Give it a name such as "nutrition app".
3. Copy the key right away. It is shown only once.
4. The API is pay-as-you-go. Open the **Billing** page and add a small amount of credit.
   Each food sentence is a very small request. Check the Console for current prices.
5. Keep the key private. Do not put it in code, a screenshot, or GitHub.

## Part 3 - Open and run the project
1. Unzip `NutritionTrackerAI.zip`.
2. In Qt Creator choose **File -> Open File or Project** and pick `CMakeLists.txt`.
3. Select the **Desktop Qt 6.x MinGW 64-bit** kit and click **Configure Project**.
4. Click the green **Run** button.
5. In the app choose **Settings -> Set API key...** and paste your key (only the first time).
6. Type what you ate, for example `2 oz cooked rice and a 4 oz apple`, and click **Calculate**.

## If something goes wrong
| Message in the app | What to do |
|---|---|
| The API key was not accepted | Paste the key again under Settings -> Set API key |
| Could not reach Claude | Check your internet connection |
| Request was rejected ... credit balance | Add credit on the Billing page |
| Too many requests | Wait a minute and try again |

## Before the showcase
- Run it once at home with your real key and log a few foods.
- Make sure the Wi-Fi at the event works, or use the **Manual add** tab as a backup.
- Take screenshots as a backup.
