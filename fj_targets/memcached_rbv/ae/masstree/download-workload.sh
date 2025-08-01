FILE_ID="1y-UBf8CuuFgAZkUg_2b_G8zh4iF_N-mq"
FILENAME="lognormal-190M.bin" # Or your desired filename

# Step 1: Fetch the warning page and extract the uuid
# We use a temporary file to store the page content
WARNING_PAGE_CONTENT=$(wget --quiet --no-check-certificate \
                             "https://drive.google.com/uc?export=download&id=${FILE_ID}" \
                             -O -)

# Extract the UUID using grep and sed
UUID=$(echo "${WARNING_PAGE_CONTENT}" | grep -oP 'name="uuid" value="\K[^"]+')

# Ensure UUID was extracted. If not, something is wrong.
if [ -z "$UUID" ]; then
    echo "Error: Could not extract UUID from Google Drive warning page. Sharing settings might be incorrect or Google's page structure changed again."
    exit 1
fi

# Step 2: Use the extracted UUID to construct the final download URL and download the file
wget --no-check-certificate \
     "https://drive.usercontent.google.com/download?id=${FILE_ID}&export=download&confirm=t&uuid=${UUID}" \
     -O "${FILENAME}"

echo "Download attempted. Check if '${FILENAME}' exists and is complete."

