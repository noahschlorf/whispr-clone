# Homebrew Cask for VoxType
# To install: brew install --cask voxtype
#
# Note: This cask is for local use or submission to homebrew-cask
# Update the url and sha256 when releasing a new version

cask "voxtype" do
  version "1.0.0"
  sha256 :no_check  # Update with actual SHA256 after first release

  url "https://github.com/noahschlorf/whispr-clone/releases/download/v#{version}/VoxType-v#{version}-macOS.dmg"
  name "VoxType"
  desc "Voice-to-text transcription using local AI"
  homepage "https://github.com/noahschlorf/whispr-clone"

  livecheck do
    url :url
    strategy :github_latest
  end

  depends_on macos: ">= :big_sur"

  app "VoxType.app"

  postflight do
    # Remind user about accessibility permissions
    system_command "/usr/bin/osascript",
                   args: [
                     "-e",
                     'display dialog "VoxType needs Accessibility permissions to detect your hotkey.\n\nGo to: System Settings → Privacy & Security → Accessibility\n\nThen add and enable VoxType." buttons {"Open Settings", "Later"} default button "Open Settings"',
                   ],
                   sudo: false
  end

  zap trash: [
    "~/.voxtype",
    "~/.whispr",
    "~/Library/Preferences/com.voxtype.app.plist",
  ]
end
