/* swintl.c  --  Internationalization info
 
  Copyright (C) 1999  Jim Lowe 
  All rights reserved.

  COPYING TERMS AND CONDITIONS:
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */


#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swintl.h"



const struct swintl_lang_map swintl_lang_table[] = {
	
	{"C",  	""},   /* "C" */
	{"da", 	"Danish"},
	{"de", 	"German"},
	{"cs", 	"Czech"},
	{"en", 	"English"},
	{"es", 	"Spanish"},
	{"fi",	"Finnish"},
	{"fr",  "French"},
	{"hy", 	"Armenian"},
	{"hr", 	"Croatian"},
	{"hu", 	"Hungarian"},
	{"in", 	"Indonesian"},
	{"is", 	"Icelandic"},
	{"it", 	"Italian"},
	{"iw", 	"Hebrew"},
	{"ja", 	"Japanese"},
	{"ji", 	"Yiddish"},
	{"jw", 	"Javanese"},
	{"ko", 	"Korean"},
	{"no", 	"Norwegian"},
	{"pl",	"Polish"},
	{"pt",	"Portuguese"},
	{"pt_BR","pt_BR"},
	{"ru", 	"Russian"},
	{"sk", 	"Slovak"},
	{"sv", 	"Swedish"},
	{"tr", 	"Turkish"},
	{"zh", 	"Chinese"},
	{(char *)NULL, (char*)(NULL)}
	/* please add more */
};


char * swintl_get_lang_code (char * lang_name)
{
	int i=0;
	while (swintl_lang_table[i].language_code_ != NULL) {
		if (!strcmp (lang_name, swintl_lang_table[i].language_name_))
			return  swintl_lang_table[i].language_code_;
		i++;
	}
	return NULL;
}


char * swintl_get_lang_name (char * lang_code)
{
	int i=0;
	while (swintl_lang_table[i].language_code_ != NULL) {
		if (!strcmp (lang_code, swintl_lang_table[i].language_code_))
			return  swintl_lang_table[i].language_name_;
		i++;
	}
	return NULL;
}

/* --------------------------------------------------------------------------
****************************************************************************
****************************************************************************

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>

<head>
<meta http-equiv="Content-Type"
content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="Microsoft FrontPage 2.0">
<title>ISO 639 Language Codes</title>
</head>

<body bgcolor="#FFFFFF">

<hr>

<h1>ISO 639 Language Codes</h1>

<hr>

<p>Technical contents of ISO 639:1988 (E/F) &quot;Code for the
representation of names of languages&quot;. </p>

<ul>
    <li>Typed by Keld.Simonsen@dkuug.dk 1990-11-30 </li>
    <li>Minor corrections, 1992-09-08 by Keld Simonsen </li>
    <li>Sundanese corrected, 1992-11-11 by Keld Simonsen </li>
</ul>

<p>Two-letter lower-case symbols are used. The Registration
Authority for ISO 639 is Infoterm, Osterreiches Normungsinstitut
(ON), Postfach 130, A-1021 Vienna, Austria. </p>

<pre>
  aa Afar
  ab Abkhazian
  af Afrikaans
  am Amharic
  ar Arabic
  as Assamese
  ay Aymara
  az Azerbaijani

  ba Bashkir
  be Byelorussian
  bg Bulgarian
  bh Bihari
  bi Bislama
  bn Bengali; Bangla
  bo Tibetan
  br Breton

  ca Catalan
  co Corsican
  cs Czech
  cy Welsh

  da Danish
  de German
  dz Bhutani

  el Greek
  en English
  eo Esperanto
  es Spanish
  et Estonian
  eu Basque

  fa Persian
  fi Finnish
  fj Fiji
  fo Faeroese
  fr French
  fy Frisian

  ga Irish
  gd Scots Gaelic
  gl Galician
  gn Guarani
  gu Gujarati

  ha Hausa
  hi Hindi
  hr Croatian
  hu Hungarian
  hy Armenian

  ia Interlingua
  ie Interlingue
  ik Inupiak
  in Indonesian
  is Icelandic
  it Italian
  iw Hebrew

  ja Japanese
  ji Yiddish
  jw Javanese

  ka Georgian
  kk Kazakh
  kl Greenlandic
  km Cambodian
  kn Kannada
  ko Korean
  ks Kashmiri
  ku Kurdish
  ky Kirghiz

  la Latin
  ln Lingala
  lo Laothian
  lt Lithuanian
  lv Latvian, Lettish

  mg Malagasy
  mi Maori
  mk Macedonian
  ml Malayalam
  mn Mongolian
  mo Moldavian
  mr Marathi
  ms Malay
  mt Maltese
  my Burmese

  na Nauru
  ne Nepali
  nl Dutch
  no Norwegian

  oc Occitan
  om (Afan) Oromo
  or Oriya

  pa Punjabi
  pl Polish
  ps Pashto, Pushto
  pt Portuguese

  qu Quechua

  rm Rhaeto-Romance
  rn Kirundi
  ro Romanian
  ru Russian
  rw Kinyarwanda

  sa Sanskrit
  sd Sindhi
  sg Sangro
  sh Serbo-Croatian
  si Singhalese
  sk Slovak
  sl Slovenian
  sm Samoan
  sn Shona
  so Somali
  sq Albanian
  sr Serbian
  ss Siswati
  st Sesotho
  su Sundanese
  sv Swedish
  sw Swahili

  ta Tamil
  te Tegulu
  tg Tajik
  th Thai
  ti Tigrinya
  tk Turkmen
  tl Tagalog
  tn Setswana
  to Tonga
  tr Turkish
  ts Tsonga
  tt Tatar
  tw Twi

  uk Ukrainian
  ur Urdu
  uz Uzbek

  vi Vietnamese
  vo Volapuk

  wo Wolof

  xh Xhosa

  yo Yoruba

  zh Chinese
  zu Zulu
</pre>

<hr>
</body>
</html>

****************************************************************************
****************************************************************************
---------------------------------------------------------------------------*/






