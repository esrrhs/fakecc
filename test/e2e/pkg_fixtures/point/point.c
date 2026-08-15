/* A user package that exports a typedef'd type plus a function taking it by
 * value.  The current package mechanism only exposes typedef'd types through
 * `pkg.Type` (struct tags are not imported as type names), so Pt must be a
 * typedef for `point.Pt` to resolve in an importing TU. */
package point;

typedef struct Pt { int x; int y; } Pt;

int sum(Pt p) { return p.x + p.y; }
