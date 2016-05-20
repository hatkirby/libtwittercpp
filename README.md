# libtwitter++
libtwitter++ is a C++ Twitter client library I'm working on to replace twitcurl, which didn't quite fit my needs. libtwitter++ is intended to be object-oriented and simple to use. For technical reasons, the repository is called libtwittercpp.

It is still under active development, and gains new features as I need them. Currently, libtwitter++ can:
- Authenticate with an OAuth consumer key, consumer secret, access key, and access secret.
- Upload media.
- Send tweets. Tweets can contain media. Tweets can be responses to other tweets.
- Consume a user stream with no parameters. libtwitter++ consumes the user stream in a separate thread. You can (should) define a callback function that will be called when the stream receives a message. The connection follows Twitter's guidelines for backoff reconnection, and for network stalling. Received tweet and user objects currently only contain limited information (IDs, authors, and text for tweets; IDs, screen names, and real names for users).
- Follow and unfollow users.
- Access friends and followers lists.
