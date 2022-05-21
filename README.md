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

If a CSV file is given to the program, it will assume that the file is generated from [ta-tooling](https://github.com/krerkkiat/ta-tooling) program, and
will look for email handles in the `emailHandle` column. It will also append `@ohio.edu` to the handles automatically. 

## Note

Do note that the `GH_TOKEN` has to be from the owner of the organization.

This program interacts with the [Organization Member of GitHub API](https://docs.github.com/en/rest/orgs/members#create-an-organization-invitation).

## Todo

- [ ] Implement the team id lookup program.

