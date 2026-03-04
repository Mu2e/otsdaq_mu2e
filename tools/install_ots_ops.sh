#!/bin/bash

# Exit immediately if a command fails
set -e

# ==========================================
# Argument Parsing
# ==========================================
if [ -z "$1" ]; then
    echo "Error: No target directory specified."
    echo "Usage: $0 <target_directory>"
    exit 1
fi

TARGET_DIR="$1"

# ==========================================
# Configuration Variables
# ==========================================
SPACK_SCRIPT_URL="https://raw.githubusercontent.com/Mu2e/otsdaq_mu2e/refs/heads/develop/tools/mu2e-quick-spack-start_v0.28.sh"
SPACK_SCRIPT_NAME="mu2e-quick-spack-start.sh"
CONFIG_REPO="git@github.com:Mu2e/otsdaq_mu2e_config.git"
SECRETS_DIR="$HOME/.secrets"

SUBSYSTEMS=("shift" "trigger" "cfo" "calo" "crv" "stm" "dcs" "tracker")

# ==========================================
# Workspace Setup
# ==========================================
echo "Setting up workspace in: $TARGET_DIR"
if [ -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' already exists. Aborting to prevent overwriting."
    exit 1
fi

mkdir "$TARGET_DIR"
cd "$TARGET_DIR"

# ==========================================
# Repository Access Check (Run Before Spack)
# ==========================================
echo "Cloning otsdaq-mu2e-config..."
# Temporarily disable exit-on-error so we can catch the git clone failure
set +e 
git clone "$CONFIG_REPO" otsdaq-mu2e-config
CLONE_STATUS=$?
set -e # Re-enable exit-on-error

if [ $CLONE_STATUS -ne 0 ]; then
    echo ""
    echo "======================================================================"
    echo "ERROR: Could not clone the configuration repository."
    echo "This usually means you do not have the correct GitHub SSH key set up or lack access."
    echo ""
    echo "Please follow these steps to set up your SSH key:"
    echo "  1. cd ~/.ssh/"
    echo "  2. ssh-keygen  # Name your key (e.g., id_username_rsa) so multiple users can coexist."
    echo "                 # Note: For shared accounts, password-protecting your key is highly recommended!"
    echo "  3. cat id_username_rsa.pub"
    echo "  4. Copy the entire file output into GitHub (Settings -> SSH and GPG keys)."
    echo "  5. To start using GitHub without extra Mu2e script help, run:"
    echo "     export GIT_SSH_COMMAND=\"ssh -i ~/.ssh/id_username_rsa\""
    echo ""
    echo "Once completed, please delete the '$TARGET_DIR' directory and run this script again."
    echo "======================================================================"
    exit 1
fi

# ==========================================
# Spack Environment Setup
# ==========================================
echo "Downloading Spack start script..."
curl -L -s "$SPACK_SCRIPT_URL" -o "$SPACK_SCRIPT_NAME"
chmod +x "$SPACK_SCRIPT_NAME"

# Clear Spack root if it exists
unset SPACK_ROOT 

echo "Running Spack setup (this may take a while)..."
./"$SPACK_SCRIPT_NAME" --all-packages --develop --dev-only --dev-artdaq --dev-otsdaq

# ==========================================
# Configuration and Secrets
# ==========================================
echo "Setting up OTS scripts..."
if [ -f "setup_ots.sh" ]; then
    mv setup_ots.sh old.setup_ots.sh
fi
ln -s otsdaq-mu2e-config/hwdev_spack_fast_setup_ots.sh setup_ots.sh

# Check for secrets, warn if missing
echo "Checking for database setup secrets..."
if [ -f "$SECRETS_DIR/db_setup_ots.sh" ] && [ -f "$SECRETS_DIR/mongodb_setup.sh" ]; then
    cp "$SECRETS_DIR/db_setup_ots.sh" ./
    cp "$SECRETS_DIR/mongodb_setup.sh" ./
    echo "Secrets copied successfully."
else
    echo "WARNING: Secrets not found in $SECRETS_DIR."
    echo "Please download the setup scripts manually from DocDB (50596):"
    echo "Link: https://mu2e-docdb.fnal.gov/cgi-bin/sso/RetrieveFile?docid=50596"
    read -p "Press Enter to continue once the files are in the current directory, or Ctrl+C to abort..."
fi

# ==========================================
# Subsystem Processing
# ==========================================
for subsystem in "${SUBSYSTEMS[@]}"; do
    echo "----------------------------------------"
    echo "Processing subsystem: $subsystem"
    echo "----------------------------------------"

    USER_DATA="Data_$subsystem"
    mkdir -p "$USER_DATA/ServiceData/"
    
    # Copy and link configuration files
    cp otsdaq-mu2e-config/CoreTableInfoNames.dat "$USER_DATA/ServiceData/"
    cp "otsdaq-mu2e-config/Data_${subsystem}/ServiceData/ActiveTableGroups.cfg" "$USER_DATA/ServiceData/ActiveTableGroups.cfg"
    ln -sf "../../otsdaq-mu2e-config/webPortTranslation.dat" "$USER_DATA/ServiceData/webPortTranslation.dat"

    # Execute the DAQ setup inside a subshell `(...)` 
    # This ensures that sourced environment variables don't pollute the next iteration
    (
        source setup_ots.sh "$subsystem"
        UpdateOTS.sh --tables
        ots --wiz 
        yes Y | ots -k
    )

    echo "Finished $subsystem."
done

echo "========================================"
echo "All subsystems processed successfully!"
echo "========================================"
