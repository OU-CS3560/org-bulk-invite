# Sending Bulk Invitation to GitHub Organization

Sending GitHub organization invitation to multiple email addresses.

## Build the executable

``` console
mkdir build
cmake -S standalone -B build/standalone
cmake --build build/standalone
```

To use the tool, create the `.env` file with the given `env.sample`, and populate
it with appropriate values. The following command will then run the program
to send an invite to all the email addresses listed in `email_addresses.txt`.

```console
./build/standalone/BulkInvite -f email_addresses.txt
```

## Note

Do note that the `GH_TOKEN` has to be from the owner of the organization.

This program interacts with the [Organization Member of GitHub API](https://docs.github.com/en/rest/orgs/members#create-an-organization-invitation).
