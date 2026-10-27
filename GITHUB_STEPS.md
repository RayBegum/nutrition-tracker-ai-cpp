# Putting this project on GitHub

## One-time setup
1. Make a free account at https://github.com (if you don't have one).
2. Install Git: https://git-scm.com/downloads
3. Tell Git who you are (use the same email as your GitHub account):
   ```
   git config --global user.name "Rahenuma Begum"
   git config --global user.email "your-github-email@example.com"
   ```

## Create the repository on GitHub
1. On github.com click **+** (top right) -> **New repository**.
2. Name: `nutrition-tracker-ai-cpp`. Keep it **Public**.
3. Do NOT tick "Add a README" (this project already has one). Click **Create repository**.

## Upload your project
Open a terminal inside the NutritionTrackerAI folder, then run:
```
git init
git add .
git commit -m "Initial commit: Nutrition Tracker v1 with Claude"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/nutrition-tracker-ai-cpp.git
git push -u origin main
```
Replace YOUR-USERNAME with your GitHub username. GitHub may ask you to sign in
(use a Personal Access Token as the password, or sign in through the browser window).

## After that, every time you change something
```
git add .
git commit -m "Describe what you changed"
git push
```

## Tips for the showcase
- Pin the repo on your GitHub profile (Profile -> Customize your pins).
- Add the repo link to your resume under Projects.

## IMPORTANT: never upload your API key
- Your key is saved by the app on your computer, not in the project folder, so it is not in these files.
- If you ever paste a key into a file in this folder, remove it before uploading.
- If a key is ever shown publicly, go to console.anthropic.com -> API keys and delete it, then make a new one.
