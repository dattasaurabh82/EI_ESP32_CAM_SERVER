# README

## To test locally

We would need ssl support

```bash
# Install mkcert (macOS example)
brew install mkcert
mkcert -install

# Create certificate for localhost
mkcert localhost 127.0.0.1

# Start HTTPS server with the certificate, from this directory
npx http-server --ssl --cert localhost+1.pem --key localhost+1-key.pem
```

## Do not forget to
Add this to your [`.gitignore`](../.gitignore)
```
# Local SSL Test certs
webflasher/*.pem
```
