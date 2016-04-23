YoutubeDS makes it possible to play Youtube videos on the ds.

It currently has no gui yet, so a preprogrammed video id is played.

[![Youtube DS](http://img.youtube.com/vi/NL1321zp1HI/0.jpg)](http://www.youtube.com/watch?v=NL1321zp1HI)

The following libraries are used:
- HappyHttp
- RapidJson
- NanoJPEG
- Helix Fixed-point HE-AAC decoder

Because I have not been able (yet) to implement https, the youtube api calls have to be routed through some kind of https->http proxy (because one can not use the youtube api without https). The folder 'https_removal' contains a php file that does just that. For a little bit of extra security, put your own youtube api key in there to prevent the use of it by other people.