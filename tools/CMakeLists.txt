
cet_script(ALWAYS_COPY create_mu2e_ots_snapshot.sh)
cet_script(ALWAYS_COPY reset_mu2e_ots_snapshot.sh)

cet_script(ALWAYS_COPY create_mu2e_ops_snapshot.sh)
cet_script(ALWAYS_COPY reset_mu2e_ops_snapshot.sh)

# cet_script(ALWAYS_COPY get_snapshot_data.sh)
# cet_script(ALWAYS_COPY get_snapshot_database.sh)
install_headers()
install_source()
install_fhicl(SUBDIRS fcl)
