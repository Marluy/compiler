#include "Interpreteur.h"
#include <stdlib.h>
#include <iostream>
using namespace std;

Interpreteur::Interpreteur(ifstream & fichier) :
m_lecteur(fichier), m_table(), m_arbre(NULL) { }

void Interpreteur::analyse()
{
    m_arbre = programme(); // on lance l'analyse de la première règle
}

void Interpreteur::traduitEnCPP(ostream & cout) const
{
    cout << "#include <iostream>" << endl << endl;
    cout << "int main()" << endl << "{" << endl;

    cout << setw(1) << "\t" << "int ";
    for(int i(0); i < m_table.getTaille(); i++)
	if( m_table[i] == "<VARIABLE>" ) cout << m_table[i].getChaine() << "(0), ";
    cout << '\010' << '\010' << ";" << endl << endl;

    getArbre()->traduitEnCPP(cout, 1);

    cout << endl << setw(1) << "\t" << "return 0;" << endl;
    cout << "}" << endl;
}

void Interpreteur::tester(const string & symboleAttendu) const throw(SyntaxeException)
{// Teste si le symbole courant est égal au symboleAttendu... Si non, lève une exception
    static char messageWhat[256];
    if( m_lecteur.getSymbole() != symboleAttendu )
    {
	sprintf(messageWhat,
		"Ligne %d, Colonne %d - Erreur de syntaxe - Symbole attendu : %s - Symbole trouvé : %s",
		m_lecteur.getLigne(), m_lecteur.getColonne(),
		symboleAttendu.c_str(), m_lecteur.getSymbole().getChaine().c_str());
	throw SyntaxeException(messageWhat);
    }
}

void Interpreteur::testerEtAvancer(const string & symboleAttendu) throw(SyntaxeException)
{// Teste si le symbole courant est égal au symboleAttendu... Si oui, avance, Sinon, lève une exception
    tester(symboleAttendu);
    m_lecteur.avancer();
}

void Interpreteur::erreur(const string & message) const throw(SyntaxeException)
{// Lève une exception contenant le message et le symbole courant trouvé
    // Utilisé lorsqu'il y a plusieurs symboles attendus possibles...
    static char messageWhat[256];
    sprintf(messageWhat,
	    "Ligne %d, Colonne %d - Erreur de syntaxe - %s - Symbole trouvé : %s",
	    m_lecteur.getLigne(), m_lecteur.getColonne(), message.c_str(), m_lecteur.getSymbole().getChaine().c_str());
    throw SyntaxeException(messageWhat);
}

void Interpreteur::chercheInst()
{
    do
    {
	m_lecteur.avancer();
    }
    while( !(m_lecteur.getSymbole() == "<VARIABLE>" ||
	    m_lecteur.getSymbole() == "si" ||
	    m_lecteur.getSymbole() == "tantque" ||
	    m_lecteur.getSymbole() == "pour" ||
	    m_lecteur.getSymbole() == "repeter" ||
	    m_lecteur.getSymbole() == "ecrire" ||
	    m_lecteur.getSymbole() == "lire" ||
	    m_lecteur.getSymbole() == "selon" ||
	    m_lecteur.getSymbole() == "<FINDEFICHIER>") );

}

Noeud* Interpreteur::programme()
{// <programme> ::= procedure principale() <seqInst> finproc FIN_FICHIER
    testerEtAvancer("procedure");
    testerEtAvancer("principale");
    testerEtAvancer("(");
    testerEtAvancer(")");
    Noeud* sequence = seqInst();
    testerEtAvancer("finproc");
    tester("<FINDEFICHIER>");
    return sequence;
}

Noeud* Interpreteur::seqInst()
{// <seqInst> ::= <inst> { <inst> }
    NoeudSeqInst* sequence = new NoeudSeqInst();
    do
    {
	sequence->ajoute(inst());
    }
    while( m_lecteur.getSymbole() == "<VARIABLE>" ||
	    m_lecteur.getSymbole() == "si" ||
	    m_lecteur.getSymbole() == "tantque" ||
	    m_lecteur.getSymbole() == "pour" ||
	    m_lecteur.getSymbole() == "repeter" ||
	    m_lecteur.getSymbole() == "ecrire" ||
	    m_lecteur.getSymbole() == "selon" ||
	    m_lecteur.getSymbole() == "lire" );
    // Tant que le symbole courant est un début possible d'instruction...
    // Il faut compléter cette condition chaque fois qu'on rajoute une nouvelle instruction
    return sequence;
}

Noeud* Interpreteur::inst()
{// <inst> ::= <affectation> ; | <instSiRiche> | <instTantQue> | <instRepeter> ; | <instPour> | <instEcrire> ; | <instLire> ;
    if( m_lecteur.getSymbole() == "<VARIABLE>" )
    {
	Noeud *affect = affectation();
	testerEtAvancer(";");
	return affect;

    }
    else if( m_lecteur.getSymbole() == "si" ) return instSiRiche();

    else if( m_lecteur.getSymbole() == "tantque" ) return instTantQue();

    else if( m_lecteur.getSymbole() == "repeter" )
    {
	Noeud* repet = instRepeter();
	testerEtAvancer(";");
	return repet;
    }
    else if( m_lecteur.getSymbole() == "pour" ) return instPour();

    else if( m_lecteur.getSymbole() == "ecrire" )
    {
	Noeud* ecrire = instEcrire();
	testerEtAvancer(";");
	return ecrire;
    }
    else if( m_lecteur.getSymbole() == "lire" )
    {
	Noeud* lire = instLire();
	testerEtAvancer(";");
	return lire;
    }
    else if( m_lecteur.getSymbole() == "selon" ) return instSelon();
    else erreur("Instruction incorrecte");
}

Noeud* Interpreteur::affectation()
{// <affectation> ::= <variable> = <expression> 
    try
    {
	tester("<VARIABLE>");

	Noeud* var = m_table.chercheAjoute(m_lecteur.getSymbole()); // La variable est ajoutée à la table et on la mémorise
	m_lecteur.avancer();
	testerEtAvancer("=");

	Noeud* exp = expression(); // On mémorise l'expression trouvée
	return new NoeudAffectation(var, exp); // On renvoie un noeud affectation
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::expression()
{// <expression> ::= <terme> { + <terme> | - <terme> }
    Noeud* term = terme();

    while( m_lecteur.getSymbole() == "+" || m_lecteur.getSymbole() == "-" )
    {
	Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
	m_lecteur.avancer();
	Noeud* factDroit = terme(); // On mémorise l'opérande droit
	term = new NoeudOperateurBinaire(operateur, term, factDroit); // Et on construuit un noeud opérateur binaire
    }
    return term; // On renvoie fact qui pointe sur la racine de l'expression
}

Noeud* Interpreteur::terme()
{// <terme> ::= <facteur> { * <facteur> | / <facteur> }
    Noeud* fact = facteur();

    while( m_lecteur.getSymbole() == "*" || m_lecteur.getSymbole() == "/" )
    {
	Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
	m_lecteur.avancer();
	Noeud* factDroit = facteur(); // On mémorise l'opérande droit
	fact = new NoeudOperateurBinaire(operateur, fact, factDroit); // Et on construuit un noeud opérateur binaire
    }

    return fact;
}

Noeud* Interpreteur::facteur()
{// <facteur> ::= <entier> | <variable> | - <expBool> | non <expBool> | ( <expBool> )
    try
    {
	Noeud* fact = NULL;
	if( m_lecteur.getSymbole() == "<VARIABLE>" || m_lecteur.getSymbole() == "<ENTIER>" )
	{// <entier> | <variable>
	    fact = m_table.chercheAjoute(m_lecteur.getSymbole()); // on ajoute la variable ou l'entier à la table
	    //fact = expBool();
	    m_lecteur.avancer();
	}
	else if( m_lecteur.getSymbole() == "-" )
	{ // - <expBool>
	    m_lecteur.avancer();
	    // on représente le moins unaire (- facteur) par une soustraction binaire (0 - facteur)
	    fact = new NoeudOperateurBinaire(Symbole("-"), m_table.chercheAjoute(Symbole("0")), facteur());
	}
	else if( m_lecteur.getSymbole() == "non" )
	{ // non <expBool>
	    m_lecteur.avancer();
	    // on représente le moins unaire (- facteur) par une soustractin binaire (0 - facteur)
	    fact = new NoeudOperateurBinaire(Symbole("non"), expBool(), NULL);
	}
	else if( m_lecteur.getSymbole() == "(" )
	{ // (<expBool>)
	    m_lecteur.avancer();
	    fact = expBool();
	    testerEtAvancer(")");
	}
	else erreur("Facteur incorrect");
	return fact;
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::expBool()
{// <expBool> ::= <relationET> { ou <relationEt> }
    Noeud* expBool = relationET();

    while( m_lecteur.getSymbole() == "ou" )
    {
	Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
	m_lecteur.avancer();
	Noeud* factDroit = relationET(); // On mémorise l'opérande droit
	expBool = new NoeudOperateurBinaire(operateur, expBool, factDroit); // Et on construuit un noeud opérateur binaire
    }

    return expBool;
}

Noeud* Interpreteur::relationET()
{// <relationEt> ::= <relation> { et <relation> }
    Noeud* relEt = relation();

    while( m_lecteur.getSymbole() == "et" )
    {
	Symbole operateur = m_lecteur.getSymbole(); // On mémorise le symbole de l'opérateur
	m_lecteur.avancer();
	Noeud* factDroit = relation(); // On mémorise l'opérande droit
	relEt = new NoeudOperateurBinaire(operateur, relEt, factDroit); // Et on construuit un noeud opérateur binaire
    }

    return relEt;
}

Noeud* Interpreteur::relation()
{// <relation> ::= <expression> { <opRel> <expression> }
    // <opRel> ::= == | != | < | <= | > | >=
    Noeud* rel = expression();

    while( m_lecteur.getSymbole() == "==" || m_lecteur.getSymbole() == "!=" ||
	    m_lecteur.getSymbole() == "<" || m_lecteur.getSymbole() == "<=" ||
	    m_lecteur.getSymbole() == ">" || m_lecteur.getSymbole() == ">=" )
    {
	Symbole opRel = m_lecteur.getSymbole();
	m_lecteur.avancer();
	Noeud* factDroit = expression();
	rel = new NoeudOperateurBinaire(opRel, rel, factDroit); // Et on construuit un noeud opérateur binaire
    }

    return rel;
}

Noeud* Interpreteur::instSi()
{// <instSi> ::= si ( <expression> ) <seqInst> finsi
    try
    {
	testerEtAvancer("si");

	testerEtAvancer("(");

	Noeud* condition = expression(); // On mémorise la condition

	testerEtAvancer(")");

	Noeud* sequence = seqInst(); // On mémorise la séquence d'instruction

	testerEtAvancer("finsi");

	return new NoeudInstSi(condition, sequence); // Et on renvoie un noeud Instruction Si
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }

}

Noeud* Interpreteur::instTantQue()
{// <instTantQue> ::= tantque ( <expression> ) <seqInst> fintantque
    try
    {
	testerEtAvancer("tantque");

	testerEtAvancer("(");

	Noeud* condition = expBool(); //expression();

	testerEtAvancer(")");

	Noeud* sequence = seqInst();

	testerEtAvancer("fintantque");

	return new NoeudInstTq(condition, sequence);
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::instSiRiche()
{//<instSiRiche> ::= si( <expression> ) <seqInst> { sinonsi (<expression>) <seqInst> } [sinon <seqInst>] finsi
    try
    {
	testerEtAvancer("si");

	testerEtAvancer("(");

	Noeud* conditionVraie = expBool();

	testerEtAvancer(")");

	Noeud* sequenceVraie = seqInst();
	Noeud* noeudRacine = new NoeudInstSiRiche(conditionVraie, sequenceVraie, NULL);
	Noeud* noeudPrec = noeudRacine;

	while( m_lecteur.getSymbole() == "sinonsi" )
	{
	    testerEtAvancer("sinonsi");

	    testerEtAvancer("(");

	    Noeud* conditionSinon = expBool();

	    testerEtAvancer(")");

	    Noeud* sequenceSinon = seqInst();

	    Noeud * noeud = new NoeudInstSiRiche(conditionSinon, sequenceSinon, NULL);
	    ((NoeudInstSiRiche*) noeudPrec)->setSeqFausse(noeud);
	    noeudPrec = noeud;
	}

	if( m_lecteur.getSymbole() == "sinon" )
	{
	    testerEtAvancer("sinon");
	    ((NoeudInstSiRiche*) noeudPrec)->setSeqFausse(seqInst());
	}

	testerEtAvancer("finsi");

	return noeudRacine;
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::instRepeter()
{//<instRepeter> ::= repeter <seqInst> jusqua ( <expression> )
    try
    {
	testerEtAvancer("repeter");

	Noeud* sequence = seqInst();

	testerEtAvancer("jusqua");

	testerEtAvancer("(");

	Noeud* condition = expBool();

	testerEtAvancer(")");

	return new NoeudInstRpt(condition, sequence);
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::instPour()
{//<instPour> ::= pour ( [ <affectation> ] ; <expression> ; [ <affectation> ] ) <seqInst> finpour
    try
    {
	testerEtAvancer("pour");

	testerEtAvancer("(");

	Noeud* init = NULL;
	if( m_lecteur.getSymbole() != ";" ) init = affectation();

	testerEtAvancer(";");

	Noeud* condition = expBool();

	testerEtAvancer(";");

	Noeud* increment = NULL;
	if( m_lecteur.getSymbole() != ")" ) increment = affectation();

	testerEtAvancer(")");

	Noeud* sequence = seqInst();

	testerEtAvancer("finpour");

	return new NoeudInstPr(init, condition, increment, sequence);
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::instLire()
{

    try
    {//<instLire> ::= lire ( <variable> { , <variable> } )
	NoeudInstLire* lire = new NoeudInstLire();

	testerEtAvancer("lire");
	tester("(");

	Noeud* var = NULL;

	do
	{
	    m_lecteur.avancer();
	    tester("<VARIABLE>");

	    var = m_table.chercheAjoute(m_lecteur.getSymbole()); // La variable est ajoutée à la table et on la mémorise
	    lire->ajoute(var);
	    m_lecteur.avancer();
	}
	while( m_lecteur.getSymbole() == "," );

	testerEtAvancer(")");

	return lire;
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::instEcrire()
{// <instEcrire> ::= ecrire ( <expression> | <chaine> { , <expression> | <chaine> } )
    try
    {
	NoeudInstEcrire* ecrire = new NoeudInstEcrire();

	testerEtAvancer("ecrire");

	tester("(");

	do
	{
	    m_lecteur.avancer();

	    Noeud * noeud(NULL);

	    if( m_lecteur.getSymbole() == "<CHAINE>" )
	    {
		noeud = m_table.chercheAjoute(m_lecteur.getSymbole());
		m_lecteur.avancer();
	    }
	    else noeud = expression();

	    ecrire->ajoute(noeud);
	}
	while( m_lecteur.getSymbole() == "," );

	testerEtAvancer(")");

	return ecrire;
    }
    catch(SyntaxeException s)
    {
	cout << "Exception levée : " << s.what() << endl;
	chercheInst();
    }
}

Noeud* Interpreteur::instSelon()
{
    // <instSelon> ::= selon(<variable>) cas <entier> : <seqInst>
    //  {cas <entier> : <seqInst>} [defaut: <seqInst>] finselon

    testerEtAvancer("selon");
    testerEtAvancer("(");
    tester("<VARIABLE>");
    Noeud* variable = m_table.chercheAjoute(m_lecteur.getSymbole());
    m_lecteur.avancer();
    testerEtAvancer(")");

    tester("cas");

    NoeudInstSelon* selon = new NoeudInstSelon(variable);



    do
    {
	Noeud* cas = NULL;

	m_lecteur.avancer();
	if( m_lecteur.getSymbole() == "<ENTIER>" )
	{
	    cas = m_table.chercheAjoute(m_lecteur.getSymbole());
	}
	else
	{
	    tester("-");
	    Symbole op = m_lecteur.getSymbole();
	    m_lecteur.avancer();
	    tester("<ENTIER>");
	    Noeud* entier = m_table.chercheAjoute(m_lecteur.getSymbole());
	    Noeud* zero = m_table.chercheAjoute(Symbole("0"));
	    cas = new NoeudOperateurBinaire(op, zero, entier);
	}

	m_lecteur.avancer();
	testerEtAvancer(":");
	Noeud* sequence = seqInst();
	selon->ajoute(cas, sequence);
    }
    while( m_lecteur.getSymbole() == "cas" );

    if( m_lecteur.getSymbole() == "defaut" )
    {
	m_lecteur.avancer();
	testerEtAvancer(":");
	selon->setDefaut(seqInst());
    }

    testerEtAvancer("finselon");

    return selon;
} 