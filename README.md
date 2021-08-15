# teaMPI with Soft Error Resilience #

See the original repository here: https://gitlab.lrz.de/hpcsoftware/teaMPI

This repository tries to integrate soft error resilience to the teaMPI library. <br>
(updating on branch ulfm\_failure\_tolerance)

We have added a single heartbeat option to send hash values of the results of the
replicas. TeaMPI handles the comparison of the hash values transparent to the application.
The user can create hash value from the biggest data structures of the application that
is being used, and include them in the single heartbeats (see [here](https://gitlab.lrz.de/AtamertRahma/towards-soft-error-resilience-in-swe-with-teampi) for usage).
