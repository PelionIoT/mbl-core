# Redirect stdout and stderr to the log file
exec >>"/var/log/wait_for_update.log" 2>&1
printf "%s: %s\n" "$(date '+%FT%T%z')" "Starting wait_for_update.sh"

set -x

wait_for_app_update_file() {
    while ! [ -e "/mnt/cache/do_app_update" ]; do
        sleep 1
    done
    rm "/mnt/cache/do_app_update"
}

update_apps() {
    mbl-app-update-manager --install-packages "$(cat /mnt/cache/firmware_path)"
    printf "%s\n" "$?" > /mnt/cache/app_update_rc
    rm /mnt/cache/firmware_path
    touch "/mnt/cache/done_app_update"
}

while true; do
    wait_for_app_update_file
    update_apps
done

# Should never get here
exit 1
