# Exercise Gridshield Participant Brief

A fictional GridVault Bank internal file server was contained after a ransomware
incident. First responders preserved the affected host and left a small evidence
package.

Your task is to determine what happened, identify the attacker infrastructure,
and recover the final proof from the attacker workstation.

Start here:

```bash
ssh investigator@127.0.0.1 -p 2221
```

Password:

```text
Inv3st!gate2024
```

Initial evidence:

```text
/evidence/endpoint.vmem
/evidence/archive.enc
/evidence/ransom_note.txt
/evidence/case_notes.txt
/evidence/volatility/
```

Begin with the memory image and correlate anything you recover with the
encrypted archive.
